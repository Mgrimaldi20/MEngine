# MEngine
Still a big WIP... It is very basic at the moment. Hoping that making it public will boost my motivation to work on it more.

## Submodules
This project uses submodules, so when cloning the project, use the `--recurse-submodules` flag to clone the submodules as well.
Alternatively, you can run `git submodule update --init --recursive` after cloning the project.
Or `git submodule init` and `git submodule update`

## Building on Windows
To build on Windows, simply run the `build_win.bat` file, it can be built in Debug or Release mode with a switch, unlike for the Unix builds, an OS switch isnt needed
`build_win.bat debug`
`build_win.bat release`

The recommended way to build on Windows however, is to use Visual Studio, open the directory there, and build in Visual Studio

## Building On Linux or MacOS
To build on Linux or MacOS, simply run the `build.sh` file, it can be built in Debug or Release mode with a switch, and the OS must be specified, eg:
`./build.sh macos debug`
`./build.sh linux release`

The semantic is as follows:
`./build [macos|linux] [debug|release]`
The script will pop up a usage message if you use it out incorrectly

The following packages may need to be installed for Linux systems:
```
sudo apt-get install libxkbcommon-dev
sudo apt-get install libxrandr-dev
sudo apt install xorg-dev
sudo apt install libglu1-mesa-dev
sudo apt install libasan6
```

## Installing the Engine
Install scripts have been created, this is how the engine will be installed for end users and the given diretory structure used. The install scripts use the same semantics as the build scripts:
`./install [macos|linux] [debug|release]`
`install_win.bat [debug|release]`

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

Otherwise, the only requirement is to create a DLL, Linux Shared Object, or a MacOS DynLib that implements the `gameservices_t` interface:
```
typedef struct
{
	int version;
	char gamename[SYS_MAX_PATH];
	char iconpath[SYS_MAX_PATH];			// games icon path
	char smiconpath[SYS_MAX_PATH];			// games icon path for small icon

	bool (*Init)(void);			// initialize game code, called once at startup
	void (*Shutdown)(void);		// shutdown game code, called once at shutdown
	void (*RunFrame)(void);		// called once per frame // TODO: add a return value that signals a command to the engine to execute an action
} gameservices_t;
```

This is an example from the DemoGame project:
```
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
		.version = 1,					// this field is for the verion of the engine that the game was built for
		.iconpath = "",					// path to the icon file for the game
		.smiconpath = "",				// path to the small icon file for the game
		.Init = Init,					// initialize game code, called once at startup
		.Shutdown = Shutdown,			// shutdown game code, called once at shutdown
		.RunFrame = RunFrame			// called once per frame
	};

	return(&gameservices);
}
```

An `mservices_t` pointer will be supplied back, this contains the engine code and engine functions that the game code may call, they are segmented into their own struct namespaces:
```
typedef struct		// logging service
{
	void (*Write)(logtype_t type, const char *msg, ...);			// write a log message MT
	void (*WriteSeq)(logtype_t type, const char *msg, ...);			// write a log message sequentially
	void (*WriteLargeSeq)(logtype_t type, const char *msg, ...);	// write a log over the max log length, will allocte heap memory
} log_t;

typedef struct		// memory cache allocator and manager
{
	void *(*Alloc)(size_t size);
	void (*Free)(void *ptr);
	void (*Reset)(void);			// reset memory cache, will free all allocated memory and NULL all pointers
	size_t (*GetMemUsed)(void);
} memcache_t;

typedef struct		// command system
{
	void (*RegisterCommand)(const char *name, cmdfunction_t function, const char *description);
	void (*RemoveCommand)(const char *name);
	void (*BufferCommand)(const cmdexecution_t exec, const char *cmd);
} cmdsystem_t;

typedef struct		// cvar system
{
	cvar_t *(*Find)(const char *name);

	cvar_t *(*RegisterString)(const char *name, const char *value, const unsigned long long flags, const char *description);
	cvar_t *(*RegisterInt)(const char *name, const int value, const unsigned long long flags, const char *description);
	cvar_t *(*RegisterFloat)(const char *name, const float value, const unsigned long long flags, const char *description);
	cvar_t *(*RegisterBool)(const char *name, const bool value, const unsigned long long flags, const char *description);

	bool (*GetString)(const cvar_t *cvar, char *out);
	bool (*GetInt)(const cvar_t *cvar, int *out);
	bool (*GetFloat)(const cvar_t *cvar, float *out);
	bool (*GetBool)(const cvar_t *cvar, bool *out);

	void (*SetString)(cvar_t *cvar, const char *value);
	void (*SetInt)(cvar_t *cvar, const int value);
	void (*SetFloat)(cvar_t *cvar, const float value);
	void (*SetBool)(cvar_t *cvar, const bool value);
} cvarsystem_t;

typedef struct		// system services
{
	bool (*Mkdir)(const char *path);
	void (*Stat)(const char *filepath, filedata_t *filedata);
	char *(*Strtok)(char *string, const char *delimiter, char **context);
	size_t (*Strlen)(const char *string, size_t maxlen);
	void (*Sleep)(unsigned long milliseconds);
	void (*Localtime)(struct tm *buf, const time_t *timer);

	thread_t *(*CreateThread)(void *(*func)(void *), void *arg);
	void (*JoinThread)(thread_t *thread);

	mutex_t *(*InitMutex)(void);
	void (*DestroyMutex)(mutex_t *mutex);
	void (*LockMutex)(mutex_t *mutex);
	void (*UnlockMutex)(mutex_t *mutex);

	condvar_t *(*CreateCondVar)(void);
	void (*DestroyCondVar)(condvar_t *condvar);
	void (*WaitCondVar)(condvar_t *condvar, mutex_t *mutex);
	void (*SignalCondVar)(condvar_t *condvar);

	void *(*LoadDLL)(const char *dllname);			// returns the DLL handle
	void (*UnloadDLL)(void *handle);
	void *(*GetProcAddress)(void *handle, const char *procname);
} sys_t;

typedef struct
{
	int version;
	log_t *log;
	memcache_t *memcache;
	cmdsystem_t *cmdsystem;
	cvarsystem_t *cvarsystem;
	sys_t *sys;
} mservices_t;
```

The API for this is not 1.0 yet, so it is subject to change.
