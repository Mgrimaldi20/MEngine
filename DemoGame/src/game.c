#include <stddef.h>
#include "../../MEngine/src/mservices.h"

#include <stdio.h>

mservices_t mservices;
gameservices_t gameservices;

static bool Init(void)
{
	(cvar_t *)mservices.cvarsystem->RegisterString("mycvar", "test", CVAR_GAME | CVAR_ARCHIVE, "this is my epic cvar");
	cvar_t* hello = mservices.cvarsystem->Find("mycvar");
	if (hello == NULL) {
		printf("%s\n", "Its null");
	} else {
		printf("%s\n", mservices.cvarsystem->GetString(hello));
		mservices.log->WriteSeq(LOG_INFO, "%s", mservices.cvarsystem->GetString(hello));
	}
	
	return(true);
}

static void Shutdown(void)
{
}

static void RunFrame(void)
{	
}

gameservices_t *GetMServices(mservices_t *services)
{
	mservices = *services;

	gameservices = (gameservices_t)		// ensure all fields are initialized
	{
		.gamename = "DemoGame",			// name of the game used by the engine for windowing
		.version = 1,					// this field is for the verion of the engine that the game was built for
		.iconpath = "",					// path to the icon file for the game
		.smiconpath = "",				// path to the small icon file for the game
		.Init = Init,					// initialize game code, called once at startup
		.Shutdown = Shutdown,			// shutdown game code, called once at shutdown
		.RunFrame = RunFrame			// called once per frame
	};

	return(&gameservices);
}
