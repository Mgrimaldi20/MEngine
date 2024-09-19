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
static uintptr_t gamedllhandle;

cfg_t *menginecfg = NULL;

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
		gamedllhandle = Sys_LoadDLL(Cfg_GetStr(menginecfg, "gGameDLL"));	// grab the library from the engine configs
		if (!gamedllhandle)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to load game DLL: %s", Cfg_GetStr(menginecfg, "gGameDLL"));
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
		.MemCache_Dump = MemCache_Dump
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
	menginecfg = Cfg_Init("MEngineConfigs.cfg");
	if (!menginecfg)
		return(false);

	if (!Log_Init())
		return(false);

	if (!MemCache_Init())
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
	MemCache_Shutdown();
	Log_Shutdown();

	if (menginecfg)
		Cfg_Shutdown(menginecfg);
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
