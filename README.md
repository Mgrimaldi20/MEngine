# MEngine
Still a big WIP... It is very basic at the moment. Hoping that making it public will boost my motivation to work on it more.

## Submodules
This project uses submodules, so when cloning the project, use the `--recurse-submodules` flag to clone the submodules as well.
Alternatively, you can run `git submodule update --init --recursive` after cloning the project.
Or `git submodule init` and `git submodule update`

## Building On Linux
To build on Linux, simply run the `build_linux.sh` file, it can be built in Debug or Release mode with a switch, eg:
`./build_linux.sh debug`
`./build_linux.sh release`

The following packages may need to be installed:
```
sudo apt-get install libxkbcommon-dev
sudo apt-get install libxrandr-dev
sudo apt install xorg-dev
sudo apt install libglu1-mesa-dev
sudo apt install libasan6
```

## Developing A Game
To develop a game you need to define a .def file that contains the following, it must export the `GetMServices` function:
```
LIBRARY <LIB NAME HERE>
EXPORTS
	GetMServices	@1
```
For example, in the DemoGame project, the file looks like this:
```
LIBRARY DemoGame
EXPORTS
	GetMServices	@1
```
Otherwise, the only requirement is to create a DLL file (Windows only for the time being) that implements the `gameservices_t` interface (to be extended upon for actual game code):
```
typedef struct
{
	int version;
	char gamename[256];
	char iconpath[260];			// games icon path
	char smiconpath[260];		// games icon path for small icon
} gameservices_t;
```
This is an example from the DemoGame project:
```
#include "../../MEngine/src/mservices.h"

mservices_t mservices;
gameservices_t gameservices;

gameservices_t *GetMServices(mservices_t *services)
{
	mservices = *services;

	gameservices = (gameservices_t)		// ensure all fields are initialized
	{
		.gamename = "DemoGame",			// name of the game used by the engine for windowing
		.version = MENGINE_VERSION,		// this field is for the verion of the engine that the game was built for
		.iconpath = "",					// path to the icon file for the game
		.smiconpath = "",				// path to the small icon file for the game
	};

	return(&gameservices);
}
```
An `mservices_t` pointer will be supplied back, this contains the engine code and engine functions that the game code may call:
```
typedef struct
{
	int version;

	// logging service
	void (*Log_Write)(logtype_t, const char *, ...);
	void (*Log_WriteSeq)(logtype_t type, const char *msg, ...);
	void (*Log_WriteLargeSeq)(logtype_t type, const char *msg, ...);

	// memory cache allocator and manager
	void *(*MemCache_Alloc)(size_t size);
	void (*MemCache_Free)(void *ptr);
	void (*MemCache_Reset)(void);
	size_t(*MemCahce_GetMemUsed)(void);
	void (*MemCache_Dump)(void);
} mservices_t;
```
