#include <stdio.h>
#include <stdlib.h>
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
	CMD_RUN_DEMO_GAME = 1 << 3,
	CMD_USE_DEF_ALLOC = 1 << 4
} cmdlineflags_t;

typedef union
{
	void *obj;
	getmservices_t func;
} funcptrobj_t;

gameservices_t gameservices;

static mservices_t mservices;
static log_t logsystem;
static cmdsystem_t cmdsystem;
static memcache_t memcache;
static cvarsystem_t cvarsystem;
static sys_t sys;

static unsigned long long cmdlineflags;
static void *gamedllhandle;

static FILE *outfp;
static FILE *errfp;

static bool gameinitialized;

/*
* Function: CreateCommandLine
* Creates a command line structure to store the command line arguments
* 
* Returns: A pointer to the new command line structure
*/
static cmdline_t *CreateCommandLine(void)
{
	cmdline_t *cmdline = malloc(sizeof(*cmdline));
	if (!cmdline)
	{
		Sys_Error("Failed to allocate memory for command line");
		return(NULL);
	}

	cmdline->args = NULL;
	cmdline->count = 0;

	return(cmdline);
}

/*
* Function: DestroyCommandLine
* Frees the memory allocated for the command line structure
* 
*	cmdline: The command line structure to free
*/
static void DestroyCommandLine(cmdline_t *cmdline)
{
	if (!cmdline)
		return;

	if (!cmdline->allocated)
		return;

	for (int i=0; i<cmdline->count; i++)
		free(cmdline->args[i]);

	free(cmdline->args);
	free(cmdline);
}

/*
* Function: ParseCommandLine
* Parses the command line arguments and sets the appropriate flags, can also call command line functions here
* 
* Returns: A boolean if the command line was parsed successfully or not
*/
static bool ParseCommandLine(void)
{
	cmdline_t *cmdline = CreateCommandLine();
	if (!cmdline)
		return(false);

	Sys_ProcessCommandLine(cmdline);

	for (int i=0; i<cmdline->count; i++)
	{
		if (!cmdline->args[i][0])
			break;

		if (strcmp(cmdline->args[i], "-help") == 0)
		{
			static const char *helpmsg = "MEngine\n"
				"Usage: MEngine [options]\n"
				"Options:\n"
				"\t-help\t\tPrint this help message\n"
				"\t-editor\t\tRun the editor\n"
				"\t-debug\t\tRun the game in debug mode\n"
				"\t-demo\t\tRun the demo game\n"
				"\t-ignoreosver\tIgnore OS version check\n"
				"\t-nocache\tDo not use the memory cache allocator\n";

			Common_Printf("%s", helpmsg);
			fflush(stdout);

			DestroyCommandLine(cmdline);

			return(false);
		}

		else if (strcmp(cmdline->args[i], "-editor") == 0)
			cmdlineflags |= CMD_MODE_EDITOR;

		else if (strcmp(cmdline->args[i], "-debug") == 0)
			cmdlineflags |= CMD_MODE_DEBUG;

		else if (strcmp(cmdline->args[i], "-ignoreosver") == 0)
			cmdlineflags |= CMD_IGNORE_OSVER;

		else if (strcmp(cmdline->args[i], "-demo") == 0)
			cmdlineflags |= CMD_RUN_DEMO_GAME;

		else if (strcmp(cmdline->args[i], "-nocache") == 0)
			cmdlineflags |= CMD_USE_DEF_ALLOC;
	}

	DestroyCommandLine(cmdline);

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
		.WriteSeq = Log_WriteSeq,
		.WriteLargeSeq = Log_WriteLargeSeq
	};

	memcache = (memcache_t)
	{
		.Alloc = MemCache_Alloc,
		.Free = MemCache_Free,
		.Reset = MemCache_Reset,
		.GetMemUsed = MemCache_GetTotalMemory,
	};

	cmdsystem = (cmdsystem_t)
	{
		.RegisterCommand = Cmd_RegisterCommand,
		.RemoveCommand = Cmd_RemoveCommand,
		.BufferCommand = Cmd_BufferCommand
	};

	cvarsystem = (cvarsystem_t)
	{
		.Find = CVar_Find,
		.RegisterString = CVar_RegisterString,
		.RegisterInt = CVar_RegisterInt,
		.RegisterFloat = CVar_RegisterFloat,
		.RegisterBool = CVar_RegisterBool,
		.GetString = CVar_GetString,
		.GetInt = CVar_GetInt,
		.GetFloat = CVar_GetFloat,
		.GetBool = CVar_GetBool,
		.SetString = CVar_SetString,
		.SetInt = CVar_SetInt,
		.SetFloat = CVar_SetFloat,
		.SetBool = CVar_SetBool
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
	cvar_t *gamedll = NULL;

	if (Common_RunDemoGame())
		gamedll = CVar_Find("g_demogamedll");

	else
		gamedll = CVar_Find("g_gamedll");	// get the game DLL name from the CVar system, designed to be overriden by the client

	if (!gamedll)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to find game DLL name in CVar system");
		return(false);
	}

	char gamedllname[CVAR_MAX_STR_LEN] = { 0 };
	if (!CVar_GetString(gamedll, gamedllname))
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get game DLL name from CVar system");
		return(false);
	}

	gamedllhandle = Sys_LoadDLL(gamedllname);
	if (!gamedllhandle)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to load game DLL: %s", gamedllname);
		return(false);
	}

	CreateMServices();		// populate the mservices struct with the function pointers

	void *procaddr = Sys_GetProcAddress(gamedllhandle, "GetMServices");
	funcptrobj_t conv =
	{
		.obj = procaddr
	};

	getmservices_t GetMServices = conv.func;

	if (!GetMServices)
	{
		Sys_Error("Failed to get GetMServices function, handshake failed, could not load function pointers");
		return(false);
	}

	gameservices = *GetMServices(&mservices);

	if (gameservices.version != MENGINE_VERSION)
	{
		Log_WriteSeq(LOG_ERROR, "Game DLL version and Engine version mismatch: (Game: %d), (Engine: %d)", gameservices.version, MENGINE_VERSION);
		return(false);
	}

	if (!Render_Init())
	{
		Log_WriteSeq(LOG_ERROR, "Failed to initialize renderer");
		return(false);
	}

	if (!gameservices.Init())	// run the games startup code
	{
		Log_WriteSeq(LOG_ERROR, "Failed to initialize the game");
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
	outfp = NULL;
	errfp = NULL;

#if defined(MENGINE_DEBUG)
	if (Sys_Mkdir("logs"))		// if this cant be done, just forget about this
	{
		char outfilename[SYS_MAX_PATH] = { 0 };
		char errfilename[SYS_MAX_PATH] = { 0 };

		snprintf(outfilename, SYS_MAX_PATH, "logs/stdout.log");
		snprintf(errfilename, SYS_MAX_PATH, "logs/stderr.log");

		(FILE *)outfp = fopen(outfilename, "w");	// truncate for each run
		(FILE *)errfp = fopen(errfilename, "w");
	}
#endif

	if (!ParseCommandLine())
		return(false);

	if (!MemCache_Init())
		return(false);

	if (!Log_Init())
		return(false);

	if (!MemCache_UseCache())
		Log_WriteSeq(LOG_WARN, "Not enough system memory for the memory cache, using the default allocator");

	else
		Log_WriteSeq(LOG_INFO, "Memory Cache allocated [bytes: %zu]", MemCache_GetTotalMemory());

	if (!Cmd_Init())
		return(false);

	if (!CVar_Init())
		return(false);

	if (!FileSys_Init())
		return(false);

	if (!Sys_Init())
		return(false);

	if (!Input_Init())
		return(false);

	if (!Event_Init())
		return(false);

	if (!InitGame())
		return(false);

	Log_WriteSeq(LOG_INFO, "Engine initialized successfully...");

	return(true);
}

/*
* Function: Common_Shutdown
* Shuts down the engine and all the subsystems
*/
void Common_Shutdown(void)
{
	Log_WriteSeq(LOG_INFO, "Engine shutting down...");

	ShutdownGame();
	Event_Shutdown();
	Input_Shutdown();
	Sys_Shutdown();
	FileSys_Shutdown();
	CVar_Shutdown();
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
int Common_Printf(const char *msg, ...)
{
	va_list argptr;
	char buffer[LOG_MAX_LEN] = { 0 };

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	int res = printf("%s\n", buffer);

	if (outfp)
		fprintf(outfp, "%s\n", buffer);

	return(res);
}

/*
* Function: Common_Errorf
* Prints an error message to the console and to the log file
* 
*	msg: The message to print
* 
* Returns: The number of characters printed
*/
int Common_Errorf(const char *msg, ...)
{
	va_list argptr;
	char buffer[LOG_MAX_LEN] = { 0 };

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	int res = fprintf(stderr, "%s\n", buffer);

	if (errfp)
		fprintf(errfp, "%s\n", buffer);

	return(res);
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
* Function: Common_RunDemoGame
* Returns if the engine is running the demo game
*/
bool Common_RunDemoGame(void)
{
	return((cmdlineflags & CMD_RUN_DEMO_GAME));
}

/*
* Function: Common_UseDefaultAlloc
* Returns if the engine is using the default allocator instead of the memory cache
*/
bool Common_UseDefaultAlloc(void)
{
	return((cmdlineflags & CMD_USE_DEF_ALLOC));
}
