#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"

typedef struct cvarentry
{
	cvar_t *value;
	struct cvarentry *next;
} cvarentry_t;

static FILE *cvarfile;

static bool HandleConversionErrors(const char *value, const char *end)
{
	if (value == end)
	{
		Log_Write(LOG_ERROR, "Failed to convert string to int: %s", value);
		return(false);
	}

	if (errno == ERANGE)
	{
		Log_Write(LOG_ERROR, "%s: Failed to convert string to int: %s", strerror(errno), value);
		return(false);
	}

	if ((errno == 0) && end && (end != 0))
	{
		Log_Write(LOG_ERROR, "Invalid characters in string, additional characters remain: %s", value);
		return(false);
	}

	if (errno != 0)
	{
		Log_Write(LOG_ERROR, "%s: An error occured: %s", value, strerror(errno));
		return(false);
	}

	return(true);
}

bool CVar_Init(void)
{
	const char *cvardir = "configs";

	if (!Sys_Mkdir(cvardir))
		return(false);

	char cvarfullname[SYS_MAX_PATH] = { 0 };
	snprintf(cvarfullname, sizeof(cvarfullname), "%s/%s", cvardir, "MEngine.cfg");

	cvarfile = fopen(cvarfullname, "r");
	if (!cvarfile)
	{
		cvarfile = fopen(cvarfullname, "w+");	// try to just recreate file, will lose cvars if file cant be read properly
		if (!cvarfile)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to open cvar file: %s", cvarfullname);
			return(false);
		}
	}

	cvarlist = NULL;

	// read the cvar file and populate the cvar list if cvars exist and if the file exists
	char line[1024] = { 0 };
	while (fgets(line, sizeof(line), cvarfile))
	{
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ')
			continue;

		char *name = NULL;
		char *value = NULL;

		name = Sys_Strtok(line, " \t\n\r", &value);

		if (!name || !value)
		{
			Log_WriteSeq(LOG_ERROR, "Invalid cvar parsed, line: %s", line);
			continue;
		}

		size_t slen = strlen(value);
		if (slen > MAX_CVAR_STR_LEN)
		{
			Log_WriteSeq(LOG_ERROR, "CVar value too long: %s", value);
			continue;
		}

		// remove the newline character from the value
		if (value[slen - 1] == '\n')
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
	cvarlist_t *current = cvarlist;
	while (current)
	{
		cvar_t *cvar = current->value;
		if (cvar->flags & CVAR_ARCHIVE)
		{
			switch (cvar->type)
			{
				case CVAR_BOOL:
					fprintf(cvarfile, "%s %d\n", current->value->name, cvar->value.b);
					break;

				case CVAR_INT:
					fprintf(cvarfile, "%s %d\n", current->value->name, cvar->value.i);
					break;

				case CVAR_FLOAT:
					fprintf(cvarfile, "%s %f\n", current->value->name, cvar->value.f);
					break;

				case CVAR_STRING:
					fprintf(cvarfile, "%s %s\n", current->value->name, cvar->value.s);
					break;
			}
		}

		current = current->next;
	}

	// loop through the list and free the memory
	current = cvarlist;
	while (current)
	{
		cvarlist_t *next = current->next;
		MemCache_Free(current->value);
		MemCache_Free(current);
		current = next;
	}

	cvarlist = NULL;

	if (cvarfile)
	{
		fclose(cvarfile);
		cvarfile = NULL;
	}
}

void CVar_ListAllCVars(void)
{
	cvarlist_t *current = cvarlist;
	while (current)
	{
		cvar_t *cvar = current->value;
		switch (cvar->type)
		{
			case CVAR_BOOL:
				Log_Write(LOG_INFO, "CVar: %s, Value: %d, Type: %d, Flags: %llu Description: %s", cvar->name, cvar->value.b, cvar->type, cvar->flags, cvar->description);
				break;

			case CVAR_INT:
				Log_Write(LOG_INFO, "CVar: %s, Value: %d, Type: %d, Flags: %llu, Description: %s", cvar->name, cvar->value.i, cvar->type, cvar->flags, cvar->description);
				break;

			case CVAR_FLOAT:
				Log_Write(LOG_INFO, "CVar: %s, Value: %f, Type: %d, Flags: %llu, Description: %s", cvar->name, cvar->value.f, cvar->type, cvar->flags, cvar->description);
				break;

			case CVAR_STRING:
				Log_Write(LOG_INFO, "CVar: %s, Value: %s, Type: %d, Flags: %llu, Description: %s", cvar->name, cvar->value.s, cvar->type, cvar->flags, cvar->description);
				break;
		}

		current = current->next;
	}
}

cvar_t *CVar_Find(const char *name)
{
	cvarlist_t *current = cvarlist;
	while (current)
	{
		if (strcmp(current->value->name, name) == 0)
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

		if (strcmp(existing->value.s, "") == 0)		// if the value is empty, overwrite it with the default value supplied by the function call
		{
			Log_Write(LOG_WARN, "CVar value is empty, overwriting with default value");
			existing->value = value;
		}

		switch (type)
		{
			case CVAR_STRING:
				snprintf(existing->value.s, sizeof(existing->value.s), "%s", value.s);
				break;

			case CVAR_INT:
			{
				errno = 0;
				char *end = NULL;

				int castval = strtol(existing->value.s, &end, 10);
				HandleConversionErrors(value.s, end);

				existing->value.i = castval;
				break;
			}

			case CVAR_FLOAT:
			{
				errno = 0;
				char *end = NULL;

				float castval = strtof(existing->value.s, &end);
				HandleConversionErrors(existing->value.s, end);

				existing->value.f = castval;
				break;
			}

			case CVAR_BOOL:
			{
				errno = 0;
				char *end = NULL;

				bool castval = strtol(existing->value.s, &end, 10);
				HandleConversionErrors(existing->value.s, end);

				existing->value.b = castval;
				break;
			}

			default:
				Sys_Error("Invalid cvar type: %d", type);
				break;
		}

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

	char *dupname = MemCache_Alloc(MAX_CVAR_STR_LEN);
	if (!dupname)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for cvar name: %s", dupname);
		MemCache_Free(cvar);
		return(NULL);
	}

	snprintf(dupname, MAX_CVAR_STR_LEN, "%s", name);

	cvar->name = dupname;
	cvar->value = value;
	cvar->type = type;
	cvar->flags = flags;
	cvar->description = description;

	cvarlist_t *listnode = MemCache_Alloc(sizeof(*listnode));
	if (!listnode)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for cvar list entry");
		MemCache_Free(cvar);
		return(NULL);
	}

	// insert the new cvar at the start of the list
	listnode->value = cvar;
	listnode->next = cvarlist;
	cvarlist = listnode;

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
	if ((!cvar) || (cvar->type != CVAR_STRING))
		return(NULL);

	return(cvar->value.s);
}

int *CVar_GetInt(cvar_t *cvar)
{
	if ((!cvar) || (cvar->type != CVAR_INT))
		return(NULL);

	return(&cvar->value.i);
}

float *CVar_GetFloat(cvar_t *cvar)
{
	if ((!cvar) || (cvar->type != CVAR_FLOAT))
		return(NULL);

	return(&cvar->value.f);
}

bool *CVar_GetBool(cvar_t *cvar)
{
	if ((!cvar) || (cvar->type != CVAR_BOOL))
		return(NULL);

	return(&cvar->value.b);
}

void CVar_SetString(cvar_t *cvar, const char *value)
{
	if ((!cvar) || (cvar->type != CVAR_STRING))
		return;

	snprintf(cvar->value.s, sizeof(cvar->value.s), "%s", value);
}

void CVar_SetInt(cvar_t *cvar, const int value)
{
	if ((!cvar) || (cvar->type != CVAR_INT))
		return;

	cvar->value.i = value;
}

void CVar_SetFloat(cvar_t *cvar, const float value)
{
	if ((!cvar) || (cvar->type != CVAR_FLOAT))
		return;

	cvar->value.f = value;
}

void CVar_SetBool(cvar_t *cvar, const bool value)
{
	if ((!cvar) || (cvar->type != CVAR_BOOL))
		return;

	cvar->value.b = value;
}
