#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "../mservices.h"
#include "../sys/sys.h"

typedef gameservices_t *(*getmservices_t)(mservices_t *services);

extern gameservices_t gameservices;

bool Common_Init(void);
void Common_Shutdown(void);
void Common_Frame(void);
bool Common_HelpMode(void);
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
void MemCache_Dump(void);
size_t MemCache_GetTotalMemory(void);
bool MemCache_UseCache(void);

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
void CVar_ListAllCVars(void);			// mostly for debugging purposes
cvar_t *CVar_Find(const char *name);
cvar_t *CVar_Register(const char *name, const cvarvalue_t value, const cvartype_t type, const unsigned long long flags, const char *description);
cvar_t *CVar_RegisterString(const char *name, const char *value, const unsigned long long flags, const char *description);
cvar_t *CVar_RegisterInt(const char *name, const int value, const unsigned long long flags, const char *description);
cvar_t *CVar_RegisterFloat(const char *name, const float value, const unsigned long long flags, const char *description);
cvar_t *CVar_RegisterBool(const char *name, const bool value, const unsigned long long flags, const char *description);
char *CVar_GetString(cvar_t *cvar);
int *CVar_GetInt(cvar_t *cvar);
float *CVar_GetFloat(cvar_t *cvar);
bool *CVar_GetBool(cvar_t *cvar);
void CVar_SetString(cvar_t *cvar, const char *value);
void CVar_SetInt(cvar_t *cvar, const int value);
void CVar_SetFloat(cvar_t *cvar, const float value);
void CVar_SetBool(cvar_t *cvar, const bool value);

bool FileSys_Init(void);
void FileSys_Shutdown(void);
