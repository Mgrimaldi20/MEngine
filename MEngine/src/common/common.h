#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "mservices.h"

typedef gameservices_t *(*getmservices_t)(mservices_t *services);

extern gameservices_t gameservices;

bool Common_Init(void);
void Common_Shutdown(void);
void Common_Frame(void);
int Common_Printf(const char *msg, ...);
int Common_Errorf(const char *msg, ...);

bool Common_EditorMode(void);
bool Common_DebugMode(void);
bool Common_IgnoreOSVer(void);
bool Common_RunDemoGame(void);
bool Common_UseDefaultAlloc(void);

#define LOG_MAX_LEN 1024

bool Log_Init(void);
void Log_Shutdown(void);
void Log_Write(const logtype_t type, const char *msg, ...);
void Log_WriteSeq(const logtype_t type, const char *msg, ...);
void Log_WriteLargeSeq(const logtype_t type, const char *msg, ...);

bool MemCache_Init(void);
void MemCache_Shutdown(void);
void *MemCache_Alloc(size_t size);
void MemCache_Free(void *ptr);
void MemCache_Reset(void);
size_t MemCahce_GetMemUsed(void);
size_t MemCache_GetTotalMemory(void);
bool MemCache_UseCache(void);

bool Cmd_Init(void);
void Cmd_Shutdown(void);
void Cmd_RegisterCommand(const char *name, cmdfunction_t function, const char *description);
void Cmd_RemoveCommand(const char *name);
void Cmd_BufferCommand(const cmdexecution_t exec, const char *cmd);
void Cmd_ExecuteCommandBuffer(void);

#define CVAR_MAX_STR_LEN 256

typedef enum
{
	CVAR_INT = 0,
	CVAR_FLOAT,
	CVAR_STRING,
	CVAR_BOOL
} cvartype_t;

typedef struct
{
	bool b;
	char s[CVAR_MAX_STR_LEN];
	int i;
	float f;
} cvarvalue_t;

struct cvar
{
	char *name;
	cvarvalue_t value;
	const char *description;
	cvartype_t type;
	unsigned long long flags;
};

bool CVar_Init(void);
void CVar_Shutdown(void);
cvar_t *CVar_Find(const char *name);
cvar_t *CVar_RegisterString(const char *name, const char *value, const unsigned long long flags, const char *description);
cvar_t *CVar_RegisterInt(const char *name, const int value, const unsigned long long flags, const char *description);
cvar_t *CVar_RegisterFloat(const char *name, const float value, const unsigned long long flags, const char *description);
cvar_t *CVar_RegisterBool(const char *name, const bool value, const unsigned long long flags, const char *description);
bool CVar_GetString(const cvar_t *cvar, char *out);
bool CVar_GetInt(const cvar_t *cvar, int *out);
bool CVar_GetFloat(const cvar_t *cvar, float *out);
bool CVar_GetBool(const cvar_t *cvar, bool *out);
void CVar_SetString(cvar_t *cvar, const char *value);
void CVar_SetInt(cvar_t *cvar, const int value);
void CVar_SetFloat(cvar_t *cvar, const float value);
void CVar_SetBool(cvar_t *cvar, const bool value);

struct filedata
{
	time_t atime;
	time_t mtime;
	time_t ctime;
	size_t filesize;
	char filename[SYS_MAX_PATH];
};

bool FileSys_Init(void);
void FileSys_Shutdown(void);
bool FileSys_FileExistsInPAK(const char *filename);
filedata_t *FileSys_ListFilesInPAK(unsigned int *numfiles, const char *filter);
bool FileSys_FileExists(const char *filename);
filedata_t *FileSys_ListFiles(unsigned int *numfiles, const char *directory, const char *filter);
void FileSys_FreeFileList(filedata_t *filelist);

bool Input_Init(void);
void Input_Shutdown(void);
void Input_ProcessKeyInput(const int key, bool down);
void Input_ClearKeyStates(void);

typedef enum
{
	EVENT_NONE = 0,
	EVENT_KEY,			// evar1 is the keycode, evar2 is an eventstate_t (key up or down)
	EVENT_MOUSE,		// evar1 is the posx and evar2 is the posy of the mouse
	EVENT_CHAR,			// evar1 stores the unicode char
} eventtype_t;

typedef enum
{
	EVENT_STATE_KEYUP = 0,
	EVENT_STATE_KEYDOWN,
} eventstate_t;

bool Event_Init(void);
void Event_Shutdown(void);
void Event_QueueEvent(const eventtype_t type, int var1, int var2);
void Event_RunEventLoop(void);
