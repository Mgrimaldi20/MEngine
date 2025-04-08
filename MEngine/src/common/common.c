#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "sys/sys.h"
#include "common.h"
#include "renderer/renderer.h"

typedef enum
{
	CMD_MODE_EDITOR = 1 << 0,
	CMD_MODE_DEBUG = 1 << 1,
	CMD_IGNORE_OSVER = 1 << 2,
	CMD_USE_DEF_ALLOC = 1 << 3
} cmdlineflags_t;

gameservices_t gameservices;

static mservices_t mservices;
static log_t logsystem;
static cmdsystem_t cmdsystem;
static memcache_t memcache;
static cvarsystem_t cvarsystem;
static filesystem_t filesystem;
static sys_t sys;

static unsigned long long cmdlineflags;
static void *gamedllhandle;

static FILE *outfp;
static FILE *errfp;
static bool isatty;

static bool gameinitialized;

/*
* Function: ProcessCommandLine
* Parses the command line arguments and sets the appropriate flags, can also call command line functions here, different from the command system
* 
* Returns: A boolean if the command line was parsed successfully or not
*/
static bool ProcessCommandLine(void)
{
	cmdline_t cmdline = { 0 };

	Sys_ParseCommandLine(&cmdline);

	for (int i=0; i<cmdline.count; i++)
	{
		if (!cmdline.args[i][0])
			break;

		if (strcmp(cmdline.args[i], "-help") == 0)
		{
			static const char *helpmsg = "MEngine\n"
				"Usage: MEngine [options]\n"
				"Options:\n"
				"\t-help\t\tPrint this help message\n"
				"\t-editor\t\tRun the editor\n"
				"\t-debug\t\tRun the game in debug mode\n"
				"\t-ignoreosver\tIgnore OS version check\n"
				"\t-nocache\tDo not use the memory cache allocator\n";

			printf("%s", helpmsg);	// this is the standard printf because its run on the command line
			fflush(stdout);

			return(false);
		}

		else if (strcmp(cmdline.args[i], "-editor") == 0)
			cmdlineflags |= CMD_MODE_EDITOR;

		else if (strcmp(cmdline.args[i], "-debug") == 0)
			cmdlineflags |= CMD_MODE_DEBUG;

		else if (strcmp(cmdline.args[i], "-ignoreosver") == 0)
			cmdlineflags |= CMD_IGNORE_OSVER;

		else if (strcmp(cmdline.args[i], "-nocache") == 0)
			cmdlineflags |= CMD_USE_DEF_ALLOC;
	}

	return(true);
}

/*
* Function: CreateMServices
* Creates the mservices struct and populates it with the function pointers
*/
static void CreateMServices(void)
{
	logsystem = (log_t)
	{
		.Write = Log_Write,
		.Writef = Log_Writef,
		.Writefv = Log_Writefv
	};

	memcache = (memcache_t)
	{
		.Alloc = MemCache_Alloc,
		.Free = MemCache_Free,
		.Reset = MemCache_Reset,
		.GetMemUsed = MemCache_GetTotalMemory,
		.GetTotalMemory = MemCache_GetTotalMemory
	};

	cmdsystem = (cmdsystem_t)
	{
		.RegisterCommand = Cmd_RegisterCommand,
		.RemoveCommand = Cmd_RemoveCommand,
		.BufferCommand = Cmd_BufferCommand
	};

	cvarsystem = (cvarsystem_t)
	{
		.Find = Cvar_Find,
		.RegisterString = Cvar_RegisterString,
		.RegisterInt = Cvar_RegisterInt,
		.RegisterFloat = Cvar_RegisterFloat,
		.RegisterBool = Cvar_RegisterBool,
		.GetString = Cvar_GetString,
		.GetInt = Cvar_GetInt,
		.GetFloat = Cvar_GetFloat,
		.GetBool = Cvar_GetBool,
		.SetString = Cvar_SetString,
		.SetInt = Cvar_SetInt,
		.SetFloat = Cvar_SetFloat,
		.SetBool = Cvar_SetBool
	};

	filesystem = (filesystem_t)
	{
		.FileExists = FileSys_FileExists,
		.ListFiles = FileSys_ListFiles,
		.FreeFileList = FileSys_FreeFileList
	};

	sys = (sys_t)
	{
		.Mkdir = Sys_Mkdir,
		.Stat = Sys_Stat,
		.Strtok = Sys_Strtok,
		.Strlen = Sys_Strlen,
		.Sleep = Sys_Sleep,
		.Localtime = Sys_Localtime,
		.CreateThread = Sys_CreateThread,
		.JoinThread = Sys_JoinThread,
		.InitMutex = Sys_CreateMutex,
		.DestroyMutex = Sys_DestroyMutex,
		.LockMutex = Sys_LockMutex,
		.UnlockMutex = Sys_UnlockMutex,
		.CreateCondVar = Sys_CreateCondVar,
		.DestroyCondVar = Sys_DestroyCondVar,
		.WaitCondVar = Sys_WaitCondVar,
		.SignalCondVar = Sys_SignalCondVar,
		.LoadDLL = Sys_LoadDLL,
		.UnloadDLL = Sys_UnloadDLL,
		.GetProcAddress = Sys_GetProcAddress
	};

	mservices = (mservices_t)
	{
		.version = MENGINE_VERSION,
		.log = &logsystem,
		.memcache = &memcache,
		.cmdsystem = &cmdsystem,
		.cvarsystem = &cvarsystem,
		.filesystem = &filesystem,
		.sys = &sys
	};
}

/*
* Function: InitGame
* Initializes the game DLL and runs the games startup code
* 
* Returns: A boolean if the game was initialized successfully or not
*/
static bool InitGame(void)
{
	cvar_t *gamedll = Cvar_Find("g_gamedll");	// get the game DLL name from the Cvar system, designed to be overriden by the client
	if (!gamedll)
	{
		Log_Write(LOG_ERROR, "Failed to find game DLL name in Cvar system");
		return(false);
	}

	char gamedllname[SYS_MAX_PATH] = { 0 };
	if (!Cvar_GetString(gamedll, gamedllname))
	{
		Log_Write(LOG_ERROR, "Failed to get game DLL name from Cvar system");
		return(false);
	}

	gamedllhandle = Sys_LoadDLL(gamedllname);
	if (!gamedllhandle)
	{
		Log_Writef(LOG_ERROR, "Failed to load game DLL: %s", gamedllname);
		return(false);
	}

	CreateMServices();		// populate the mservices struct with the function pointers

	prochandle_t proc;
	proc.obj = Sys_GetProcAddress(gamedllhandle, "GetMServices");
	
	getmservices_t GetMServices = (getmservices_t)proc.func;	// fun type punning
	if (!GetMServices)
	{
		Sys_Error("Failed to get GetMServices function, handshake failed, could not load function pointers");
		return(false);
	}

	gameservices = *GetMServices(&mservices);

	if (gameservices.version != MENGINE_VERSION)
	{
		Sys_Error("Game DLL version and Engine version mismatch: (Game: %d), (Engine: %d)", gameservices.version, MENGINE_VERSION);
		return(false);
	}

	if (!Render_Init())
	{
		Log_Write(LOG_ERROR, "Failed to initialize renderer");
		return(false);
	}

	if (!gameservices.Init())	// run the games startup code
	{
		Log_Write(LOG_ERROR, "Failed to initialize the game");
		return(false);
	}

	gameinitialized = true;

	return(true);
}

/*
* Function: ShutdownGame
* Shuts down the game DLL and runs the games shutdown code
*/
static void ShutdownGame(void)
{
	if (!gameinitialized)
		return;

	gameservices.Shutdown();	// run the games shutdown code

	Render_Shutdown();

	if (gamedllhandle)
	{
		Sys_UnloadDLL(gamedllhandle);
		gamedllhandle = NULL;
	}

	gameinitialized = false;
}

/*
* Function: Common_Init
* Initializes the engine and all the subsystems
* 
* Returns: A boolean if the engine was initialized successfully or not
*/
bool Common_Init(void)
{
#if defined(MENGINE_DEBUG)
	if (Sys_Mkdir("logs"))		// if this cant be done, just forget about this
	{
		char outfilename[SYS_MAX_PATH] = { 0 };
		char errfilename[SYS_MAX_PATH] = { 0 };

		snprintf(outfilename, SYS_MAX_PATH, "logs/stdout.log");
		snprintf(errfilename, SYS_MAX_PATH, "logs/stderr.log");

		outfp = fopen(outfilename, "w");	// truncate for each run
		errfp = fopen(errfilename, "w");
	}
#endif

	isatty = Sys_IsTTY();

	if (!ProcessCommandLine()
		|| !MemCache_Init()
		|| !Log_Init()
		|| !Cmd_Init()
		|| !Cvar_Init()
		|| !FileSys_Init()
		|| !Sys_Init()
		|| !Input_Init()
		|| !Event_Init()
		|| !InitGame())
	{
		Common_Errorf("Failed to initialize the engine, shutting down...");
		return(false);
	}

	if (!MemCache_UseCache())
		Log_Write(LOG_WARN, "Not enough system memory for the memory cache, using the default allocator");

	else
		Log_Writef(LOG_INFO, "Memory Cache allocated [bytes: %zu]", MemCache_GetTotalMemory());

	Log_Write(LOG_INFO, "Engine initialized successfully...");

	return(true);
}

/*
* Function: Common_Shutdown
* Shuts down the engine and all the subsystems
*/
void Common_Shutdown(void)
{
	Log_Write(LOG_INFO, "Engine shutting down...");

	ShutdownGame();
	Event_Shutdown();
	Input_Shutdown();
	Sys_Shutdown();
	FileSys_Shutdown();
	Cvar_Shutdown();
	Cmd_Shutdown();
	MemCache_Shutdown();
	Log_Shutdown();

	if (outfp)
		fclose(outfp);

	if (errfp)
		fclose(errfp);
}

/*
* Function: Common_Frame
* Happens every frame, runs the engine loop and game loop
*/
void Common_Frame(void)
{
	Event_RunEventLoop();

	gameservices.RunFrame();

	Render_StartFrame();
	Render_Frame();
	Render_EndFrame();
}

/*
* Function: Common_Printf
* Prints a message to the console and to the log file
* 
*	msg: The message to print
* 
* Returns: The number of characters printed
*/
void Common_Printf(const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	Log_Writefv(LOG_INFO, msg, argptr);

	if (outfp)
	{
		va_list argptrcpy;
		va_copy(argptrcpy, argptr);
		vfprintf(outfp, msg, argptrcpy);
		fprintf(outfp, "\n");
		va_end(argptrcpy);
	}

#if defined(MENGINE_DEBUG)
	if (isatty)
	{
		va_list argptrcpy;
		va_copy(argptrcpy, argptr);
		vprintf(msg, argptrcpy);
		printf("\n");
		va_end(argptrcpy);
	}
#endif

	va_end(argptr);
}

/*
* Function: Common_Warnf
* Prints a warning message to the console and to the log file
* 
*	msg: The message to print
* 
* Returns: The number of characters printed
*/
void Common_Warnf(const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	Log_Writefv(LOG_WARN, msg, argptr);

	if (outfp)
	{
		va_list argptrcpy;
		va_copy(argptrcpy, argptr);
		vfprintf(outfp, msg, argptrcpy);
		fprintf(outfp, "\n");
		va_end(argptrcpy);
	}

#if defined(MENGINE_DEBUG)
	if (isatty)
	{
		va_list argptrcpy;
		va_copy(argptrcpy, argptr);
		vprintf(msg, argptrcpy);
		printf("\n");
		va_end(argptrcpy);
	}
#endif

	va_end(argptr);
}

/*
* Function: Common_Errorf
* Prints an error message to the console and to the log file
* 
*	msg: The message to print
* 
* Returns: The number of characters printed
*/
void Common_Errorf(const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	Log_Writefv(LOG_ERROR, msg, argptr);

	if (errfp)
	{
		va_list argptrcpy;
		va_copy(argptrcpy, argptr);
		vfprintf(errfp, msg, argptrcpy);
		fprintf(errfp, "\n");
		fflush(errfp);
		va_end(argptrcpy);
	}

	if (isatty)
	{
		va_list argptrcpy;
		va_copy(argptrcpy, argptr);
		vfprintf(stderr, msg, argptrcpy);
		fprintf(stderr, "\n");
		va_end(argptrcpy);
	}

	va_end(argptr);
}

/*
* Function: Common_EditorMode
* Returns if the engine is in editor mode
*/
bool Common_EditorMode(void)
{
	return((cmdlineflags & CMD_MODE_EDITOR));
}

/*
* Function: Common_DebugMode
* Returns if the engine is in debug mode
*/
bool Common_DebugMode(void)
{
	return((cmdlineflags & CMD_MODE_DEBUG));
}

/*
* Function: Common_IgnoreOSVer
* Returns if the engine is ignoring the OS version, used by the system
*/
bool Common_IgnoreOSVer(void)
{
	return((cmdlineflags & CMD_IGNORE_OSVER));
}

/*
* Function: Common_UseDefaultAlloc
* Returns if the engine is using the default allocator instead of the memory cache
*/
bool Common_UseDefaultAlloc(void)
{
	return((cmdlineflags & CMD_USE_DEF_ALLOC));
}
