#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sys/sys.h"
#include "common.h"
#include "renderer/renderer.h"

typedef enum
{
	CMD_MODE_HELP = 0,
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
static memcache_t memcache;
static cvarsystem_t cvarsystem;
static sys_t sys;

static unsigned long long cmdlineflags;
static void *gamedllhandle;

static void ParseCommandLine(void)
{
	char cmdline[SYS_MAX_CMDLINE_ARGS][SYS_MAX_CMDLINE_ARGS];
	Sys_ProcessCommandLine(cmdline);

	for (int i=0; i<SYS_MAX_CMDLINE_ARGS; i++)
	{
		if (!cmdline[i][0])
			break;

		if (!strcmp(cmdline[i], "help"))
			cmdlineflags |= CMD_MODE_HELP;

		else if (!strcmp(cmdline[i], "editor"))
			cmdlineflags |= CMD_MODE_EDITOR;

		else if (!strcmp(cmdline[i], "debug"))
			cmdlineflags |= CMD_MODE_DEBUG;

		else if (!strcmp(cmdline[i], "ignoreosver"))
			cmdlineflags |= CMD_IGNORE_OSVER;

		else if (!strcmp(cmdline[i], "demo"))
			cmdlineflags |= CMD_RUN_DEMO_GAME;

		else if (!strcmp(cmdline[i], "nocache"))
			cmdlineflags |= CMD_USE_DEF_ALLOC;
	}
}

static void PrintHelpMsg(void)
{
	const char *helpmsg = "MEngine\n"
		"Usage: MEngine [options]\n"
		"Options:\n"
		"\t-help\t\tPrint this help message\n"
		"\t-editor\t\tRun the editor\n"
		"\t-debug\t\tRun the game in debug mode\n"
		"\t-demo\t\tRun the demo game\n"
		"\t-ignoreosver\tIgnore OS version check\n"
		"\t-nocache\tDo not use the memory cache allocator\n"
		"Press any key to exit...\n";

	printf("%s", helpmsg);
	Log_WriteSeq(LOG_INFO, helpmsg);

	fflush(stdout);
}

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
		.Dump = MemCache_Dump
	};

	cvarsystem = (cvarsystem_t)
	{
		.ListAllCVars = CVar_ListAllCVars,
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
		.Strtok = Sys_Strtok,
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
		.cvarsystem = &cvarsystem,
		.sys = &sys
	};
}

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

	char *gamedllname = CVar_GetString(gamedll);
	if (!gamedllname)
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

	return(true);
}

static void ShutdownGame(void)
{
	gameservices.Shutdown();	// run the games shutdown code

	Render_Shutdown();

	Sys_UnloadDLL(gamedllhandle);
	gamedllhandle = 0;
}

static void UpdateConfigs(void)
{
	// TODO: update any config changes, key binds, etc.
}

bool Common_Init(void)
{
	if (!MemCache_Init())
		return(false);

	if (!Log_Init())
		return(false);

	if (!MemCache_UseCache())
		Log_WriteSeq(LOG_WARN, "Not enough system memory for the memory cache, using the default allocator");

	else
		Log_WriteSeq(LOG_INFO, "Memory Cache allocated [bytes: %zu]", MemCache_GetTotalMemory());

	if (!CVar_Init())
		return(false);

	ParseCommandLine();

	if (Common_HelpMode())
	{
		PrintHelpMsg();
		return(false);		// just print the help message and exit, return false even though the operation was successful to shut down the engine
	}

	if (!Sys_Init())
		return(false);

	if (!InitGame())
		return(false);

	return(true);
}

void Common_Shutdown(void)
{
#if defined(MENGINE_DEBUG)
	CVar_ListAllCVars();
	MemCache_Dump();
#endif

	ShutdownGame();
	Sys_Shutdown();
	CVar_Shutdown();
	MemCache_Shutdown();
	Log_Shutdown();
}

void Common_Frame(void)		// happens every frame
{
	UpdateConfigs();		// update any config changes, key binds, etc.

	// also run the game loop here, and any on frame events

	Render_StartFrame();
	Render_Frame();
	Render_EndFrame();
}

bool Common_HelpMode(void)
{
	return((cmdlineflags & CMD_MODE_HELP) != 0);
}

bool Common_EditorMode(void)
{
	return((cmdlineflags & CMD_MODE_EDITOR) != 0);
}

bool Common_DebugMode(void)
{
	return((cmdlineflags & CMD_MODE_DEBUG) != 0);
}

bool Common_IgnoreOSVer(void)
{
	return((cmdlineflags & CMD_IGNORE_OSVER) != 0);
}

bool Common_RunDemoGame(void)
{
	return((cmdlineflags & CMD_RUN_DEMO_GAME) != 0);
}

bool Common_UseDefaultAlloc(void)
{
	return((cmdlineflags & CMD_USE_DEF_ALLOC) != 0);
}
