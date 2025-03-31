#include <stddef.h>
#include "../../MEngine/src/mservices.h"

mservices_t mservices;
gameservices_t gameservices;

static bool Init(void)
{
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
		.version = "0.13.256",			// this field is for the verion of the engine that the game was built for
		.iconpath = "",					// path to the icon file for the game
		.smiconpath = "",				// path to the small icon file for the game
		.Init = Init,					// initialize game code, called once at startup
		.Shutdown = Shutdown,			// shutdown game code, called once at shutdown
		.RunFrame = RunFrame			// called once per frame
	};

	return(&gameservices);
}
