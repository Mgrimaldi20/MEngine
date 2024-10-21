#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "../renderer/renderer.h"

typedef enum
{
	CMD_MODE_HELP = 0,
	CMD_MODE_EDITOR = 1 << 0,
	CMD_IGNORE_OSVER = 1 << 1,
	CMD_RUN_DEMO_GAME = 1 << 2
} cmdlineflags_t;

gameservices_t gameservices;

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

		else if (!strcmp(cmdline[i], "ignoreosver"))
			cmdlineflags |= CMD_IGNORE_OSVER;

		else if (!strcmp(cmdline[i], "demo"))
			cmdlineflags |= CMD_RUN_DEMO_GAME;
	}
}

static mservices_t CreateMServices(void)
{
	mservices_t mservices =
	{
		.version = MENGINE_VERSION,
		.Log_Write = Log_Write,
		.Log_WriteSeq = Log_WriteSeq,
		.Log_WriteLargeSeq = Log_WriteLargeSeq,
		.MemCache_Alloc = MemCache_Alloc,
		.MemCache_Free = MemCache_Free,
		.MemCache_Reset = MemCache_Reset,
		.MemCahce_GetMemUsed = MemCahce_GetMemUsed,
		.MemCache_Dump = MemCache_Dump,
		.CVar_ListAllCVars = CVar_ListAllCVars,
		.CVar_Find = CVar_Find,
		.CVar_RegisterString = CVar_RegisterString,
		.CVar_RegisterInt = CVar_RegisterInt,
		.CVar_RegisterFloat = CVar_RegisterFloat,
		.CVar_RegisterBool = CVar_RegisterBool,
		.CVar_GetString = CVar_GetString,
		.CVar_GetInt = CVar_GetInt,
		.CVar_GetFloat = CVar_GetFloat,
		.CVar_GetBool = CVar_GetBool,
		.CVar_SetString = CVar_SetString,
		.CVar_SetInt = CVar_SetInt,
		.CVar_SetFloat = CVar_SetFloat,
		.CVar_SetBool = CVar_SetBool,
		.Sys_Mkdir = Sys_Mkdir,
		.Sys_Strtok = Sys_Strtok,
		.Sys_Sleep = Sys_Sleep,
		.Sys_Localtime = Sys_Localtime,
		.Sys_LoadDLL = Sys_LoadDLL,
		.Sys_UnloadDLL = Sys_UnloadDLL,
		.Sys_GetProcAddress = Sys_GetProcAddress
	};

	return(mservices);
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

	mservices_t mservices = CreateMServices();

	getmservices_t GetMServices = (getmservices_t)Sys_GetProcAddress(gamedllhandle, "GetMServices");
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
		Sys_Error("Failed to initialize renderer");
		return(false);
	}

	return(true);
}

static void ShutdownGame(void)
{
	Render_Shutdown();

	Sys_UnloadDLL(gamedllhandle);
	gamedllhandle = 0;
}

static void UpdateConfigs(void)
{
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

	if (!Sys_Init())
		return(false);

	if (!Common_HelpMode())
	{
		if (!InitGame())
			return(false);
	}

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

void Common_PrintHelpMsg(void)
{
	const char *helpmsg = "MEngine\n"
		"Usage: MEngine [options]\n"
		"Options:\n"
		"\t-help\t\tPrint this help message\n"
		"\t-editor\t\tRun the editor\n"
		"\t-demo\t\tRun the demo game\n"
		"\t-ignoreosver\tIgnore OS version check\n"
		"Press any key to exit...\n";

	printf(helpmsg);
	Log_WriteSeq(LOG_INFO, helpmsg);
}

bool Common_HelpMode(void)
{
	return((cmdlineflags & CMD_MODE_HELP) != 0);
}

bool Common_EditorMode(void)
{
	return((cmdlineflags & CMD_MODE_EDITOR) != 0);
}

bool Common_IgnoreOSVer(void)
{
	return((cmdlineflags & CMD_IGNORE_OSVER) != 0);
}

bool Common_RunDemoGame(void)
{
	return((cmdlineflags & CMD_RUN_DEMO_GAME) != 0);
}
