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

static void ParseCommandLine(char cmdlinein[MAX_CMDLINE_ARGS][MAX_CMDLINE_ARGS])
{
	for (int i=0; i<MAX_CMDLINE_ARGS; i++)
	{
		if (!cmdlinein[i][0])
			break;

		if (!strcmp(cmdlinein[i], "help"))
			cmdlineflags |= CMD_MODE_HELP;

		else if (!strcmp(cmdlinein[i], "editor"))
			cmdlineflags |= CMD_MODE_EDITOR;

		else if (!strcmp(cmdlinein[i], "ignoreosver"))
			cmdlineflags |= CMD_IGNORE_OSVER;

		else if (!strcmp(cmdlinein[i], "demo"))
			cmdlineflags |= CMD_RUN_DEMO_GAME;
	}
}

static bool InitGame(void)
{
	if (Common_RunDemoGame())
	{
		gamedllhandle = Sys_LoadDLL("DemoGame.dll");
		if (!gamedllhandle)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to load DemoGame.dll");
			return(false);
		}
	}

	else
	{
		cvar_t *gamedll = CVar_Find("g_gamedll");	// get the game DLL name from the CVar system, designed to be overriden by the client

		if (!gamedll)
			return(false);

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
	}

	mservices_t mservices;
	getmservices_t GetMServices;

	mservices = (mservices_t)
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
		.CVar_SetBool = CVar_SetBool
	};

	GetMServices = (getmservices_t)Sys_GetProcAddress(gamedllhandle, "GetMServices");
	if (!GetMServices)
	{
		Sys_Error("Failed to get GetMServices function, handshake failed, could not load function pointers");
		return(false);
	}

	gameservices = *GetMServices(&mservices);

	if (gameservices.version != MENGINE_VERSION)
	{
		Log_WriteSeq(LOG_ERROR, "Game DLL version and Engine version mismatch: %d", gameservices.version);
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

bool Common_Init(char cmdlinein[MAX_CMDLINE_ARGS][MAX_CMDLINE_ARGS])
{
	if (!MemCache_Init())
		return(false);

	if (!Log_Init())
		return(false);

	Log_WriteSeq(LOG_INFO, "Memory Cache allocated [bytes: %zu]", MemCache_GetTotalMemory());

	if (!CVar_Init())
		return(false);

	ParseCommandLine(cmdlinein);

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

bool Common_IgnoreOSVer(void)
{
	return((cmdlineflags & CMD_IGNORE_OSVER) != 0);
}

bool Common_RunDemoGame(void)
{
	return((cmdlineflags & CMD_RUN_DEMO_GAME) != 0);
}
