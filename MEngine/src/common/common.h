#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "../mservices.h"
#include "../sys/sys.h"

typedef gameservices_t *(*getmservices_t)(mservices_t *services);

extern gameservices_t gameservices;

#define MAX_CMDLINE_ARGS 32

bool Common_Init(char cmdlinein[MAX_CMDLINE_ARGS][MAX_CMDLINE_ARGS]);
void Common_Shutdown(void);
void Common_Frame(void);
bool Common_HelpMode(void);
bool Common_EditorMode(void);
bool Common_IgnoreOSVer(void);
bool Common_RunDemoGame(void);

#define LOG_MAX_LEN 1024

bool Log_Init(void);
void Log_Shutdown(void);
void Log_Write(logtype_t type, const char *msg, ...);
void Log_WriteSeq(logtype_t type, const char *msg, ...);
void Log_WriteLargeSeq(logtype_t type, const char *msg, ...);

bool MemCache_Init(void);
void MemCache_Shutdown(void);
void *MemCache_Alloc(size_t size);
void MemCache_Free(void *ptr);
void MemCache_Reset(void);
size_t MemCahce_GetMemUsed(void);
void MemCache_Dump(void);

#define CFG_TABLE_SIZE 256

typedef struct cfg
{
	char *key;
	char *value;
	char *lnremaining;
	char filename[SYS_MAX_PATH];
	size_t keylen;
	size_t valuelen;
	struct cfg *table[CFG_TABLE_SIZE];
	struct cfg *next;
} cfg_t;

extern cfg_t *menginecfg;

cfg_t *Cfg_Init(const char *filename);
void Cfg_Shutdown(cfg_t *cfg);
char *Cfg_GetStr(cfg_t *cfg, const char *key);
long Cfg_GetNum(cfg_t *cfg, const char *key);
unsigned long Cfg_GetUNum(cfg_t *cfg, const char *key);
long long Cfg_GetLNum(cfg_t *cfg, const char *key);
unsigned long long Cfg_GetULNum(cfg_t *cfg, const char *key);
float Cfg_GetFloat(cfg_t *cfg, const char *key);
double Cfg_GetDouble(cfg_t *cfg, const char *key);
long double Cfg_GetLDouble(cfg_t *cfg, const char *key);
bool Cfg_SetStr(cfg_t *cfg, const char *key, const char *val);
bool Cfg_SetNum(cfg_t *cfg, const char *key, const long val);
bool Cfg_SetUNum(cfg_t *cfg, const char *key, const unsigned long val);
bool Cfg_SetLNum(cfg_t *cfg, const char *key, const long long val);
bool Cfg_SetULNum(cfg_t *cfg, const char *key, const unsigned long long val);
bool Cfg_SetFloat(cfg_t *cfg, const char *key, const float val);
bool Cfg_SetDouble(cfg_t *cfg, const char *key, const double val);
bool Cfg_SetLDouble(cfg_t *cfg, const char *key, const long double val);
