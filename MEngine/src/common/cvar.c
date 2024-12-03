#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "sys/sys.h"
#include "common.h"

#define DEF_CVAR_MAP_CAPACITY 256

typedef struct cvarentry
{
	cvar_t *value;
	struct cvarentry *next;
} cvarentry_t;

typedef struct
{
	size_t numcvars;
	size_t capacity;
	cvarentry_t **cvars;
} cvarmap_t;

static cvarmap_t *cvarmap;
static FILE *cvarfile;

static const char *cvardir = "configs";
static const char *cvarfilename = "MEngine.cfg";
static const char *override_filename = "overrides.cfg";

static size_t HashFunction(const char *name)	// this function is actually okay for resizing, a new hash is generated for the new map size
{
	size_t hash = 0;
	size_t len = strnlen(name, CVAR_MAX_STR_LEN);

	for (size_t i=0; i<len; i++)
		hash = (hash * 31) + name[i];

	return(hash % cvarmap->capacity);
}

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

	if ((errno == 0) && end && (end != 0) && (strcmp(end, "") != 0))
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

static void ListAllCVars(void)
{
	Log_WriteSeq(LOG_INFO, "\t\tCVar Dump [number of cvars: %zu, cvar map capacity: %zu]", cvarmap->numcvars, cvarmap->capacity);

	size_t numcvars = 0;
	for (size_t i=0; i<cvarmap->capacity; i++)
	{
		cvarentry_t *current = cvarmap->cvars[i];
		while (current)
		{
			cvar_t *cvar = current->value;
			switch (cvar->type)
			{
				case CVAR_BOOL:
					Log_WriteSeq(LOG_INFO, "\t\t\tCVar: %s, Value: %d, Type: %d, Flags: %llu Description: %s", cvar->name, cvar->value.b, cvar->type, cvar->flags, cvar->description);
					break;

				case CVAR_INT:
					Log_WriteSeq(LOG_INFO, "\t\t\tCVar: %s, Value: %d, Type: %d, Flags: %llu, Description: %s", cvar->name, cvar->value.i, cvar->type, cvar->flags, cvar->description);
					break;

				case CVAR_FLOAT:
					Log_WriteSeq(LOG_INFO, "\t\t\tCVar: %s, Value: %f, Type: %d, Flags: %llu, Description: %s", cvar->name, cvar->value.f, cvar->type, cvar->flags, cvar->description);
					break;

				case CVAR_STRING:
					Log_WriteSeq(LOG_INFO, "\t\t\tCVar: %s, Value: %s, Type: %d, Flags: %llu, Description: %s", cvar->name, cvar->value.s, cvar->type, cvar->flags, cvar->description);
					break;
			}

			numcvars++;
			current = current->next;
		}
	}

	Log_WriteSeq(LOG_INFO, "\t\tEnd of CVar Dump");
}

/*
* Function: get_name_value
* Gets a name and value pair from the config file. Ignores comments and leading whitespace.
*
*   line: The line to parse
*   length: The length of the line
*   name: The first word in the name-value pair
*   value: The second term in the name-value pair, enclosed in double quotes
*
* Returns: A boolean if it's acceptable or not
*/
bool get_name_value(char* line, int length, char* name, char* value)
{
	bool acceptable = true;

	// This changes to true when we are inside quotes
	bool reading_value = false;

	int i = 0, ni = 0, vi = 0;
	while (i < length && line[i] != '\0')
	{
		if (line[i] == '"')
		{
			if (reading_value)
			{
				// End quote reached, save the value
				break;
			}
			else
			{
				// Start quote reached, save the name
				reading_value = true;
			}
		}
		else if (line[i] == ' ')
		{
			if (reading_value)
			{
				// Names are not allowed to have whitespace, only add to values
				value[vi] += line[i];
				vi += 1;
			}
			else
			{
				// The next char must be a quote to be acceptable
				if (i == length - 1 || line[i + 1] != '"')
				{
					acceptable = false;
					break;
				}
			}
		}
		else if (line[i] == '#')
		{
			if (!reading_value)
			{
				// This is a comment, ignore anything after this
				acceptable = false;
				break;
			}
			else
			{
				value[vi] = line[i];
				vi += 1;
			}
		}
		else if (line[i] == '\n' || line[i] == '\r')
		{
			// We do not allow these characters
			acceptable = false;
			break;
		}
		else
		{
			if (reading_value)
			{
				value[vi] = line[i];
				vi += 1;
			}
			else
			{
				name[ni] = line[i];
				ni += 1;
			}
		}

		i++;
	}

	name[ni] = '\0';
	value[vi] = '\0';

	return acceptable;
}

/* 
* Function: read_CVars_from_file
* Inserts the CVars into the hashmap from the file given.
* 
*   cVar_file: The file with the cVars in it
*   log_message: A string to insert into the log messages so you can tell what file it came from
*/
void read_CVars_from_file(FILE *cVar_file, char *log_message) 
{
	char line[1024] = { 0 };
	while (fgets(line, sizeof(line), cVar_file))
	{
		char name[1024] = { 0 };
		char value[1024] = { 0 };

		bool acceptable = get_name_value(line, sizeof(line), name, value);

		if (!acceptable)
		{
			Log_WriteSeq(LOG_ERROR, "Line is not a %s pair, line: %s", log_message, line);
			continue;
		}

		if ((strnlen(name, 1024) == 0) || (strnlen(value, 1024) == 0))
		{
			Log_WriteSeq(LOG_ERROR, "Invalid %s parsed, line: %s", log_message, line);
			continue;
		}

		size_t slen = strnlen(value, CVAR_MAX_STR_LEN);
		if (slen > CVAR_MAX_STR_LEN)
		{
			Log_WriteSeq(LOG_ERROR, "%s value too long: %s", log_message, value);
			continue;
		}

		cvar_t* cvar = CVar_RegisterString(name, value, CVAR_NONE, "");
		if (!cvar)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to register %s: %s", log_message, name);
			continue;
		}
	}
}

bool CVar_Init(void)
{
	if (!Sys_Mkdir(cvardir))
		return(false);

	char cvarfullname[SYS_MAX_PATH] = { 0 };
	snprintf(cvarfullname, sizeof(cvarfullname), "%s/%s", cvardir, cvarfilename);

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

	cvarmap = MemCache_Alloc(sizeof(*cvarmap));
	if (!cvarmap)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for cvar map");
		fclose(cvarfile);
		return(false);
	}

	cvarmap->capacity = DEF_CVAR_MAP_CAPACITY;
	cvarmap->numcvars = 0;

	cvarmap->cvars = MemCache_Alloc(sizeof(*cvarmap->cvars) * cvarmap->capacity);
	if (!cvarmap->cvars)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for cvar map entries");
		MemCache_Free(cvarmap);
		fclose(cvarfile);
		return(false);
	}

	for (size_t i=0; i<cvarmap->capacity; i++)
		cvarmap->cvars[i] = NULL;

	// Read the cvar file and populate the cvar list if cvars exist and if the file exists
	read_CVars_from_file(cvarfile, "cvar");

	fclose(cvarfile);
	cvarfile = NULL;

	// Read the overrides file and populate the cvar list if cvars exist and if the file exists
	char override_fullname[SYS_MAX_PATH] = { 0 };
	snprintf(override_fullname, sizeof(override_fullname), "%s/%s", cvardir, override_filename);
	FILE* overrides_file = fopen(override_fullname, "r");
	if (overrides_file)
	{
		Log_WriteSeq(LOG_INFO, "Overrides file exists: %s", override_fullname);

		read_CVars_from_file(overrides_file, "overrides cVar");

		fclose(overrides_file);
		overrides_file = NULL;
	} 
	else 
	{
		Log_WriteSeq(LOG_INFO, "Overrides file does not exist: %s", override_fullname);
	}

	return(true);
}

void CVar_Shutdown(void)
{
#if defined(MENGINE_DEBUG)
	ListAllCVars();
#endif

	(void)ListAllCVars;		// unused runction error fix

	char cvarfullname[SYS_MAX_PATH] = { 0 };
	snprintf(cvarfullname, sizeof(cvarfullname), "%s/%s", cvardir, cvarfilename);

	cvarfile = fopen(cvarfullname, "w");
	if (!cvarfile)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to open cvar file: %s", cvarfullname);
		return;
	}

	// go through the map and write all the cvars to the file and free the memory
	for (size_t i=0; i<cvarmap->capacity; i++)
	{
		cvarentry_t *current = cvarmap->cvars[i];
		while (current)
		{
			cvar_t *cvar = current->value;

			if (cvar->flags & CVAR_ARCHIVE)
			{
				switch (cvar->type)
				{
					case CVAR_BOOL:
						fprintf(cvarfile, "%s \"%d\"\n", cvar->name, cvar->value.b);
						break;

					case CVAR_INT:
						fprintf(cvarfile, "%s \"%d\"\n", cvar->name, cvar->value.i);
						break;

					case CVAR_FLOAT:
						fprintf(cvarfile, "%s \"%f\"\n", cvar->name, cvar->value.f);
						break;

					case CVAR_STRING:
						fprintf(cvarfile, "%s \"%s\"\n", cvar->name, cvar->value.s);
						break;
				}
			}

			cvarentry_t *next = current->next;
			MemCache_Free(current->value->name);
			MemCache_Free(current->value);
			MemCache_Free(current);
			current = next;
		}
	}

	if (cvarfile)
	{
		fclose(cvarfile);
		cvarfile = NULL;
	}
}

cvar_t *CVar_Find(const char *name)
{
	size_t index = HashFunction(name);
	cvarentry_t *current = cvarmap->cvars[index];
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

	char *dupname = MemCache_Alloc(CVAR_MAX_STR_LEN);
	if (!dupname)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for cvar name: %s", dupname);
		MemCache_Free(cvar);
		return(NULL);
	}

	snprintf(dupname, CVAR_MAX_STR_LEN, "%s", name);

	cvar->name = dupname;
	cvar->value = value;
	cvar->type = type;
	cvar->flags = flags;
	cvar->description = description;

	size_t index = HashFunction(name);
	cvarentry_t *entry = MemCache_Alloc(sizeof(*entry));
	if (!entry)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for cvar entry: %s", name);
		MemCache_Free(cvar);
		MemCache_Free(dupname);
		return(NULL);
	}

	entry->value = cvar;
	entry->next = NULL;

	if (!cvarmap->cvars[index])
		cvarmap->cvars[index] = entry;

	else	// collision, add to start of list
	{
		entry->next = cvarmap->cvars[index];
		cvarmap->cvars[index] = entry;
	}

	cvarmap->numcvars++;

	// if the number of cvars is st 75% capacity, resize the map to double, move all the cvars to the new map, and free the old map
	if (cvarmap->numcvars >= (cvarmap->capacity * 0.75))
	{
		cvarmap->capacity *= 2;
		cvarentry_t **newcvars = MemCache_Alloc(sizeof(*newcvars) * cvarmap->capacity);
		if (!newcvars)
		{
			Log_Write(LOG_ERROR, "Failed to allocate memory for new cvar map");
			MemCache_Free(cvarmap->cvars);
			MemCache_Free(cvarmap);
			return(NULL);
		}

		for (size_t i=0; i<cvarmap->capacity; i++)
			newcvars[i] = NULL;

		for (size_t i=0; i<cvarmap->capacity/2; i++)
		{
			cvarentry_t *current = cvarmap->cvars[i];
			while (current)
			{
				cvarentry_t *next = current->next;
				size_t newindex = HashFunction(current->value->name);

				if (!newcvars[newindex])
					newcvars[newindex] = current;

				else
				{
					current->next = newcvars[newindex];
					newcvars[newindex] = current;
				}

				current = next;
			}
		}

		MemCache_Free(cvarmap->cvars);
		cvarmap->cvars = newcvars;
	}

	return(cvar);
}

cvar_t *CVar_RegisterString(const char *name, const char *value, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	snprintf(cvarvalue.s, sizeof(cvarvalue.s), "%s", value);
	return(CVar_Register(name, cvarvalue, CVAR_STRING, flags, description));
}

cvar_t *CVar_RegisterInt(const char *name, const int value, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	cvarvalue.i = value;
	return(CVar_Register(name, cvarvalue, CVAR_INT, flags, description));
}

cvar_t *CVar_RegisterFloat(const char *name, const float value, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	cvarvalue.f = value;
	return(CVar_Register(name, cvarvalue, CVAR_FLOAT, flags, description));
}

cvar_t *CVar_RegisterBool(const char *name, const bool value, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	cvarvalue.b = value;
	return(CVar_Register(name, cvarvalue, CVAR_BOOL, flags, description));
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
