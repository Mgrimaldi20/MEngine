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
static const char *overridefilename = "overrides.cfg";
static char cvarfullname[SYS_MAX_PATH];

static bool initialized;

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
* Function: GetNameValue
* Gets a name and value pair from the config file. Ignores comments and leading whitespace.
*
*	line: The line to parse
*	length: The length of the line
*	name: The first word in the name-value pair
*	value: The second term in the name-value pair, enclosed in double quotes
*
* Returns: A boolean if it's acceptable or not
*/
static bool GetNameValue(char *line, const int length, char *name, char *value)
{
	bool acceptable = true;
	bool readingvalue = false;	// this changes to true when we are inside quotes

	int i = 0, ni = 0, vi = 0;
	while (i < length && line[i] != '\0')
	{
		if (line[i] == '"')
		{
			if (readingvalue)		// end quote reached, save the value
				break;

			else
				readingvalue = true;	// start quote reached, save the name
		}

		else if (line[i] == ' ')
		{
			if (readingvalue)
			{
				value[vi] += line[i];	// names are not allowed to have whitespace, only add to values
				vi += 1;
			}

			else
			{
				if (i == length - 1 || line[i + 1] != '"')		// the next char must be a quote to be acceptable
				{
					acceptable = false;
					break;
				}
			}
		}

		else if (line[i] == '#')
		{
			if (!readingvalue)
			{
				acceptable = false;		// this is a comment, ignore anything after this
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
			acceptable = false;		// we do not allow these characters
			break;
		}

		else
		{
			if (readingvalue)
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

	return(acceptable);
}

/*
* Function: ReadCVarsFromFile
* Inserts the cvars into the hashmap from the file given.
* 
*	infile: The file with the cvars in it
*	filename: A string to insert into the log messages so you can tell what file it came from
*/
static void ReadCVarsFromFile(FILE *infile, const char *filename)
{
	char line[1024] = { 0 };
	while (fgets(line, sizeof(line), infile))
	{
		char cmdline[CMD_MAX_STR_LEN] = { 0 };

		if (line[0] == '\n' || line[0] == '\r' || line[0] == '#')
			continue;

		char *saveptr = NULL;

		char *cmdname = Sys_Strtok(line, " ", &saveptr);
		char *args = Sys_Strtok(NULL, "\n\r", &saveptr);

		char name[CVAR_MAX_STR_LEN] = { 0 };
		char value[CVAR_MAX_STR_LEN] = { 0 };

		if (!GetNameValue(args, strnlen(args, sizeof(args)), name, value))
		{
			Log_Write(LOG_ERROR, "Failed to read cvar from file: %s", filename);
			continue;
		}

		snprintf(cmdline, sizeof(cmdline), "%s %s %s", cmdname, name, value);

		Cmd_BufferCommand(CMD_EXEC_NOW, cmdline);
	}
}

static void Seta_Cmd(const cmdargs_t *args)
{
	if (args->argc != 3)
	{
		Log_Write(LOG_INFO, "Usage: %s <cvarname> \"<value>\"", args->argv[0], args->argc);
		return;
	}

	if (!CVar_RegisterString(args->argv[1], args->argv[2], CVAR_NONE, ""))
		Log_Write(LOG_ERROR, "Failed to set cvar: %s", args->argv[1]);
}

static void Seti_Cmd(const cmdargs_t *args)
{
	if (args->argc != 3)
	{
		Log_Write(LOG_INFO, "Usage: %s <cvarname> \"<value>\"", args->argv[0], args->argc);
		return;
	}

	errno = 0;
	char *end = NULL;

	int castval = strtol(args->argv[2], &end, 10);
	HandleConversionErrors(args->argv[2], end);

	if (!CVar_RegisterInt(args->argv[1], castval, CVAR_NONE, ""))
		Log_Write(LOG_ERROR, "Failed to set cvar: %s", args->argv[1]);
}

static void Setf_Cmd(const cmdargs_t *args)
{
	if (args->argc != 3)
	{
		Log_Write(LOG_INFO, "Usage: %s <cvarname> \"<value>\"", args->argv[0], args->argc);
		return;
	}

	errno = 0;
	char *end = NULL;

	float castval = strtof(args->argv[2], &end);
	HandleConversionErrors(args->argv[2], end);

	if (!CVar_RegisterFloat(args->argv[1], castval, CVAR_NONE, ""))
		Log_Write(LOG_ERROR, "Failed to set cvar: %s", args->argv[1]);
}

static void Setb_Cmd(const cmdargs_t *args)
{
	if (args->argc != 3)
	{
		Log_Write(LOG_INFO, "Usage: %s <cvarname> \"<value>\"", args->argv[0], args->argc);
		return;
	}

	errno = 0;
	char *end = NULL;

	bool castval = strtol(args->argv[2], &end, 10);
	HandleConversionErrors(args->argv[2], end);

	if (!CVar_RegisterBool(args->argv[1], castval, CVAR_NONE, ""))
		Log_Write(LOG_ERROR, "Failed to set cvar: %s", args->argv[1]);
}

bool CVar_Init(void)
{
	if (initialized)
		return(true);

	Cmd_RegisterCommand("seta", Seta_Cmd, "Sets a cvar to a string value");
	Cmd_RegisterCommand("seti", Seti_Cmd, "Sets a cvar to an integer value");
	Cmd_RegisterCommand("setf", Setf_Cmd, "Sets a cvar to a float value");
	Cmd_RegisterCommand("setb", Setb_Cmd, "Sets a cvar to a boolean value");

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

	if (!Sys_Mkdir(cvardir))
		return(false);

	snprintf(cvarfullname, sizeof(cvarfullname), "%s/%s", cvardir, cvarfilename);

	// read the cvar file and populate the cvar list if cvars exist
	if (!FileSys_FileExists(cvarfullname))
	{
		cvarfile = fopen(cvarfullname, "w+");	// try to just recreate file, will lose cvars if file cant be read properly
		if (!cvarfile)
		{
			Log_WriteSeq(LOG_ERROR, "CVar file does not exist and cannot be recreated: %s", cvarfullname);
			return(false);
		}

		fclose(cvarfile);	// close the opened file if created
		cvarfile = NULL;
	}

	cvarfile = fopen(cvarfullname, "r");
	if (!cvarfile)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to open cvar file: %s", cvarfullname);
		return(false);
	}

	ReadCVarsFromFile(cvarfile, cvarfullname);

	fclose(cvarfile);
	cvarfile = NULL;

	// read the overrides file and populate the cvar list if cvars exist and if the file exists
	if (FileSys_FileExists(overridefilename))
	{
		FILE *overridesfile = fopen(overridefilename, "r");
		if (!overridesfile)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to open overrides file: %s", overridefilename);
			return(false);
		}

		Log_WriteSeq(LOG_INFO, "Reading data from the overrides file: %s", overridefilename);

		ReadCVarsFromFile(overridesfile, overridefilename);

		fclose(overridesfile);
		overridesfile = NULL;
	}

	initialized = true;

	return(true);
}

void CVar_Shutdown(void)
{
	if (!initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down cvar system");

#if defined(MENGINE_DEBUG)
	ListAllCVars();
#endif

	(void)ListAllCVars;		// gets rid of unused function warning

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
						fprintf(cvarfile, "setb %s \"%d\"\n", cvar->name, cvar->value.b);
						break;

					case CVAR_INT:
						fprintf(cvarfile, "seti %s \"%d\"\n", cvar->name, cvar->value.i);
						break;

					case CVAR_FLOAT:
						fprintf(cvarfile, "setf %s \"%f\"\n", cvar->name, cvar->value.f);
						break;

					case CVAR_STRING:
						fprintf(cvarfile, "seta %s \"%s\"\n", cvar->name, cvar->value.s);
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

	fclose(cvarfile);
	cvarfile = NULL;

	MemCache_Free(cvarmap->cvars);
	MemCache_Free(cvarmap);

	initialized = false;
}

cvar_t *CVar_Find(const char *name)
{
	if (!name || !name[0])
		return(NULL);

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
	if (!name || !name[0])
	{
		Log_Write(LOG_WARN, "Failed to register cvar, invalid cvar name: CVar name could be empty");
		return(NULL);
	}

	cvar_t *existing = CVar_Find(name);	// check if the cvar already exists and update its params if it does
	if (existing)
	{
		Log_Write(LOG_INFO, "CVar already exists: (%s): updating new parameters", name);

		if (strcmp(existing->value.s, "") == 0)		// if the value is empty, overwrite it with the default value supplied by the function call
		{
			Log_Write(LOG_WARN, "CVar value is empty, overwriting with default value");
			existing->value = value;
		}

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

	size_t index = HashFunction(name);

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
	if ((!cvar) || (cvar->type != CVAR_STRING) || (cvar->flags & CVAR_READONLY))
		return;

	snprintf(cvar->value.s, sizeof(cvar->value.s), "%s", value);
}

void CVar_SetInt(cvar_t *cvar, const int value)
{
	if ((!cvar) || (cvar->type != CVAR_INT) || (cvar->flags & CVAR_READONLY))
		return;

	cvar->value.i = value;
}

void CVar_SetFloat(cvar_t *cvar, const float value)
{
	if ((!cvar) || (cvar->type != CVAR_FLOAT) || (cvar->flags & CVAR_READONLY))
		return;

	cvar->value.f = value;
}

void CVar_SetBool(cvar_t *cvar, const bool value)
{
	if ((!cvar) || (cvar->type != CVAR_BOOL) || (cvar->flags & CVAR_READONLY))
		return;

	cvar->value.b = value;
}
