#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"

#define MAX_CVAR_NAME 64

typedef struct cvarlist
{
	char key[MAX_CVAR_NAME];
	cvar_t *value;
	struct cvarlist *next;
} cvarlist_t;

static cvarlist_t *cvarlist;
static FILE *cvarfile;

bool CVar_Init(void)
{
	const char *cvardir = "configs";

	if (!Sys_Mkdir(cvardir))
		return(false);

	char cvarfullname[SYS_MAX_PATH] = { 0 };
	snprintf(cvarfullname, sizeof(cvarfullname), "%s/%s", cvardir, "MEngine.cfg");

	cvarfile = fopen(cvarfullname, "w+");
	if (!cvarfile)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to open cvar file: %s", cvarfullname);
		return(false);
	}

	// create the cvar list
	cvarlist = MemCache_Alloc(sizeof(*cvarlist));
	if (!cvarlist)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for cvar map");
		return(false);
	}

	cvarlist->key[0] = '\0';
	cvarlist->value = NULL;
	cvarlist->next = NULL;

	// read the cvar file and populate the cvar list if cvars exist and if the file exists
	char line[1024] = { 0 };
	while (fgets(line, sizeof(line), cvarfile))
	{
		char *name = NULL;
		char *value = NULL;

		name = Sys_Strtok(line, " ", &value);

		if (!name || !value)
			continue;

		// remove the newline character from the value
		size_t slen = strlen(value);
		if (slen > MAX_CVAR_STR_LEN)
		{
			Log_WriteSeq(LOG_ERROR, "CVar value too long: (%s): , skipping", value);
			continue;
		}

		value[slen - 1] = '\0';
		cvarvalue_t cvarvalue = { 0 };
		snprintf(cvarvalue.s, sizeof(cvarvalue.s), "%s", value);

		cvar_t *cvar = CVar_Register(name, cvarvalue, CVAR_STRING, CVAR_NONE, "");
		if (!cvar)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to register cvar: %s", name);
			continue;
		}
	}

	fclose(cvarfile);
	cvarfile = NULL;

	return(true);
}

void CVar_Shutdown(void)
{
	const char *cvarfullname = "configs/MEngine.cfg";
	cvarfile = fopen("configs/MEngine.cfg", "w");
	if (!cvarfile)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to open cvar file: %s", cvarfullname);
		return;
	}

	// go through the list and write the cvars to the file
	cvarlist_t *current = cvarlist->next;
	while (current)
	{
		cvar_t *cvar = current->value;
		if (cvar->flags & CVAR_ARCHIVE)
		{
			switch (cvar->type)
			{
				case CVAR_BOOL:
					fprintf(cvarfile, "%s %d\n", current->key, cvar->value.b);
					break;

				case CVAR_INT:
					fprintf(cvarfile, "%s %d\n", current->key, cvar->value.i);
					break;

				case CVAR_FLOAT:
					fprintf(cvarfile, "%s %f\n", current->key, cvar->value.f);
					break;

				case CVAR_STRING:
					fprintf(cvarfile, "%s %s\n", current->key, cvar->value.s);
					break;
			}
		}

		current = current->next;
	}

	// loop through the list and free the memory
	current = cvarlist->next;
	while (current)
	{
		cvarlist_t *next = current->next;
		MemCache_Free(current->value);
		MemCache_Free(current);
		current = next;
	}

	MemCache_Free(cvarlist);

	if (cvarfile)
	{
		fclose(cvarfile);
		cvarfile = NULL;
	}
}

cvar_t *CVar_Find(const char *name)
{
	cvarlist_t *current = cvarlist->next;
	while (current)
	{
		if (!strcmp(current->key, name))
			return(current->value);

		current = current->next;
	}

	return(NULL);
}

cvar_t *CVar_Register(const char *name, const cvarvalue_t value, const cvartype_t type, const unsigned long long flags, const char *description)
{
	cvar_t *existing = CVar_Find(name);
	if (existing)
	{
		Log_Write(LOG_INFO, "CVar already exists: (%s): updating new parameters", name);
		existing->value = value;
		existing->type = type;
		existing->flags = flags;
		existing->description = description;
		return(existing);
	}

	cvar_t *cvar = MemCache_Alloc(sizeof(*cvar));
	if (!cvar)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for cvar: %s", name);
		return(NULL);
	}

	cvar->name = name;
	cvar->value = value;
	cvar->type = type;
	cvar->flags = flags;
	cvar->description = description;

	cvarlist_t *newcvar = MemCache_Alloc(sizeof(*newcvar));
	if (!newcvar)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for cvar list entry");
		MemCache_Free(cvar);
		return(NULL);
	}

	snprintf(newcvar->key, sizeof(newcvar->key), "%s", name);
	newcvar->value = cvar;
	newcvar->next = cvarlist->next;
	cvarlist->next = newcvar;

	return(cvar);
}

cvar_t *CVar_RegisterString(const char *name, const char *value, const cvartype_t type, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	snprintf(cvarvalue.s, sizeof(cvarvalue.s), "%s", value);
	return(CVar_Register(name, cvarvalue, type, flags, description));
}

cvar_t *CVar_RegisterInt(const char *name, const int value, const cvartype_t type, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	cvarvalue.i = value;
	return(CVar_Register(name, cvarvalue, type, flags, description));
}

cvar_t *CVar_RegisterFloat(const char *name, const float value, const cvartype_t type, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	cvarvalue.f = value;
	return(CVar_Register(name, cvarvalue, type, flags, description));
}

cvar_t *CVar_RegisterBool(const char *name, const bool value, const cvartype_t type, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	cvarvalue.b = value;
	return(CVar_Register(name, cvarvalue, type, flags, description));
}

char *CVar_GetString(cvar_t *cvar)
{
	if (cvar->type != CVAR_STRING)
		return(NULL);

	return(cvar->value.s);
}

int *CVar_GetInt(cvar_t *cvar)
{
	if (cvar->type != CVAR_INT)
		return(NULL);

	return(&cvar->value.i);
}

float *CVar_GetFloat(cvar_t *cvar)
{
	if (cvar->type != CVAR_FLOAT)
		return(NULL);

	return(&cvar->value.f);
}

bool *CVar_GetBool(cvar_t *cvar)
{
	if (cvar->type != CVAR_BOOL)
		return(NULL);

	return(&cvar->value.b);
}

void CVar_SetString(cvar_t *cvar, const char *value)
{
	if (cvar->type != CVAR_STRING)
		return;

	snprintf(cvar->value.s, sizeof(cvar->value.s), "%s", value);
}

void CVar_SetInt(cvar_t *cvar, const int value)
{
	if (cvar->type != CVAR_INT)
		return;

	cvar->value.i = value;
}

void CVar_SetFloat(cvar_t *cvar, const float value)
{
	if (cvar->type != CVAR_FLOAT)
		return;

	cvar->value.f = value;
}

void CVar_SetBool(cvar_t *cvar, const bool value)
{
	if (cvar->type != CVAR_BOOL)
		return;

	cvar->value.b = value;
}
