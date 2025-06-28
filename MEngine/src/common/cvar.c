#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "sys/sys.h"
#include "common.h"

#define DEF_CVAR_MAP_CAPACITY 128
#define CVAR_MAX_STR_LEN 260
#define CVAR_MAX_LINE_LEN 520		// double the max string length

typedef enum
{
	CVAR_INT = 0,
	CVAR_FLOAT,
	CVAR_STRING,
	CVAR_BOOL
} cvartype_t;

typedef union
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
static const char *cvarfilename = "mengine.cfg";
static const char *overridefilename = "overrides.cfg";
static char cvarfullname[SYS_MAX_PATH];

static bool initialized;

/*
* Function: HashCvarName
* Hashes the name of the cvar to generate an index, using the FNV-1a algorithm
* 
*	name: The name of the cvar
* 
* Returns: The hash value
*/
static size_t HashCvarName(const char *name)
{
	size_t hash = 2166136261u;	// initial offset basis, large prime number
	size_t len = Sys_Strlen(name, CVAR_MAX_STR_LEN);

	for (size_t i=0; i<len; i++)
	{
		hash ^= (unsigned char)name[i];
		hash *= 16777619;	// FNV prime number
	}

	return(hash & (cvarmap->capacity - 1));
}

/*
* Function: HandleConversionErrors
* Handles errors that occur during string to numeric type conversion
* 
*	value: The string to convert for logging purposes
*	end: If the conversion failed, this will point to the first invalid character
* 
* Returns: A boolean if the conversion was successful or not
*/
static bool HandleConversionErrors(const char *value, const char *end)
{
	if (value == end)
	{
		Log_Writef(LOG_ERROR, "Failed to convert string to int: %s", value);
		return(false);
	}

	if (errno == ERANGE)
	{
		Log_Writef(LOG_ERROR, "%s: Failed to convert string to int: %s", strerror(errno), value);
		return(false);
	}

	if ((errno == 0) && end && (strcmp(end, "") != 0))
	{
		Log_Writef(LOG_ERROR, "Invalid characters in string, additional characters remain: %s", value);
		return(false);
	}

	if (errno != 0)
	{
		Log_Writef(LOG_ERROR, "%s: An error occured: %s", value, strerror(errno));
		return(false);
	}

	return(true);
}

/*
* Function: ListAllCvars
* Lists all the cvars in the hashmap
*/
static void ListAllCvars(void)
{
	Log_Writef(LOG_INFO, "\t\tCvar Dump [number of cvars: %zu, cvar map capacity: %zu]", cvarmap->numcvars, cvarmap->capacity);

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
					Log_Writef(LOG_INFO, "\t\t\tCvar: %s, Value: %d, Type: %d, Flags: %llu Description: %s", cvar->name, cvar->value.b, cvar->type, cvar->flags, cvar->description);
					break;

				case CVAR_INT:
					Log_Writef(LOG_INFO, "\t\t\tCvar: %s, Value: %d, Type: %d, Flags: %llu, Description: %s", cvar->name, cvar->value.i, cvar->type, cvar->flags, cvar->description);
					break;

				case CVAR_FLOAT:
					Log_Writef(LOG_INFO, "\t\t\tCvar: %s, Value: %f, Type: %d, Flags: %llu, Description: %s", cvar->name, cvar->value.f, cvar->type, cvar->flags, cvar->description);
					break;

				case CVAR_STRING:
					Log_Writef(LOG_INFO, "\t\t\tCvar: %s, Value: %s, Type: %d, Flags: %llu, Description: %s", cvar->name, cvar->value.s, cvar->type, cvar->flags, cvar->description);
					break;
			}

			numcvars++;
			current = current->next;
		}
	}

	Log_Write(LOG_INFO, "\t\tEnd of Cvar Dump");
}

/*
* Function: GetNameValue
* Gets a name and value pair from the config file. Ignores comments and leading whitespace.
* A comment is a line that starts with a # symbol
*
*	line: The line to parse
*	length: The length of the line
*	name: The first word in the name-value pair
*	value: The second term in the name-value pair, enclosed in double quotes
*
* Returns: A boolean if it's acceptable or not
*/
static bool GetNameValue(const char *line, const size_t length, char *name, char *value)
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
* Function: ReadCvarsFromFile
* Inserts the cvars into the hashmap from the file given. The input file can be a regular config file or an override file
* 
*	infile: The file with the cvars in it
*	filename: A string to insert into the log messages so you can tell what file it came from
*/
static void ReadCvarsFromFile(FILE *infile, const char *filename)
{
	char line[1024] = { 0 };
	while (fgets(line, sizeof(line), infile))
	{
		char cmdline[CMD_MAX_STR_LEN] = { 0 };

		if (line[0] == '\n' || line[0] == '\r' || line[0] == '#')
			continue;

		char *args = NULL;
		char * const cmdname = Sys_Strtok(line, " ", &args);

		if (!cmdname)
		{
			Log_Writef(LOG_ERROR, "Failed to read the command name from file: %s", filename);
			continue;
		}

		if (!args)
		{
			Log_Writef(LOG_ERROR, "Failed to read the arguments from file: %s", filename);
			continue;
		}

		char name[CVAR_MAX_STR_LEN] = { 0 };
		char value[CVAR_MAX_STR_LEN] = { 0 };

		if (!GetNameValue(args, Sys_Strlen(args, sizeof(line) - (cmdname - line)), name, value))
		{
			Log_Writef(LOG_ERROR, "Failed to read cvar from file: %s", filename);
			continue;
		}

		snprintf(cmdline, sizeof(cmdline), "%s %s %s", cmdname, name, value);

		Cmd_BufferCommand(CMD_EXEC_NOW, cmdline);
	}
}

/*
* Function: RegisterCvar
* Registers a cvar with the cvar system and adds it to the cvar hash map.
* 
*	name: The name of the cvar
*	value: The value struct of the cvar
*	type: The enum type of the cvar
*	flags: Bitfield to define the cvars properties
*	description: A short description of the cvar
* 
* Returns: A pointer to the newly registered cvar
*/
static cvar_t *RegisterCvar(const char *name, const cvarvalue_t value, const cvartype_t type, const unsigned long long flags, const char *description)
{
	if (!name || !name[0])
	{
		Log_Write(LOG_WARN, "Failed to register cvar, invalid cvar name: Cvar name could be empty");
		return(NULL);
	}

	cvar_t *existing = Cvar_Find(name);	// check if the cvar already exists and update its params if it does
	if (existing)
	{
		Log_Writef(LOG_INFO, "Cvar already exists: (%s): updating new parameters", name);
		existing->flags = flags;
		existing->description = description;
		return(existing);
	}

	cvar_t *cvar = MemCache_Alloc(sizeof(*cvar));
	if (!cvar)
	{
		Log_Writef(LOG_ERROR, "Failed to allocate memory for cvar: %s", name);
		return(NULL);
	}

	char *dupname = MemCache_Alloc(CVAR_MAX_STR_LEN);
	if (!dupname)
	{
		Log_Writef(LOG_ERROR, "Failed to allocate memory for cvar name: %s", dupname);
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
		Log_Writef(LOG_ERROR, "Failed to allocate memory for cvar entry: %s", name);
		MemCache_Free(cvar);
		MemCache_Free(dupname);
		return(NULL);
	}

	entry->value = cvar;
	entry->next = NULL;

	size_t index = HashCvarName(name);

	if (!cvarmap->cvars[index])
		cvarmap->cvars[index] = entry;

	else	// collision, add to start of list
	{
		entry->next = cvarmap->cvars[index];
		cvarmap->cvars[index] = entry;
	}

	cvarmap->numcvars++;

	if (cvarmap->numcvars >= (cvarmap->capacity * 0.75))	// if the number of cvars is at 75% capacity, resize the map to double
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
				size_t newindex = HashCvarName(current->value->name);

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

/*
* Function: Seta_Cmd
* Sets a cvar to a string value from the command line
* 
*	args: The command arguments, argv[1] is the cvar name, argv[2] is the value
*/
static void Seta_Cmd(const cmdargs_t *args)
{
	if (args->argc != 3)
	{
		Log_Writef(LOG_INFO, "Usage: %s [cvarname] \"[value]\"", args->argv[0]);
		return;
	}

	if (!Cvar_RegisterString(args->argv[1], args->argv[2], CVAR_NONE, ""))
		Log_Writef(LOG_ERROR, "Failed to set cvar: %s", args->argv[1]);
}

/*
* Function: Seti_Cmd
* Sets a cvar to an integer value from the command line
* 
* 	args: The command arguments, argv[1] is the cvar name, argv[2] is the value
*/
static void Seti_Cmd(const cmdargs_t *args)
{
	if (args->argc != 3)
	{
		Log_Writef(LOG_INFO, "Usage: %s [cvarname] \"[value]\"", args->argv[0]);
		return;
	}

	errno = 0;
	char *end = NULL;

	int castval = strtol(args->argv[2], &end, 10);
	HandleConversionErrors(args->argv[2], end);

	if (!Cvar_RegisterInt(args->argv[1], castval, CVAR_NONE, ""))
		Log_Writef(LOG_ERROR, "Failed to set cvar: %s", args->argv[1]);
}

/*
* Function: Setf_Cmd
* Sets a cvar to a float value from the command line
* 
* 	args: The command arguments, argv[1] is the cvar name, argv[2] is the value
*/
static void Setf_Cmd(const cmdargs_t *args)
{
	if (args->argc != 3)
	{
		Log_Writef(LOG_INFO, "Usage: %s [cvarname] \"[value]\"", args->argv[0]);
		return;
	}

	errno = 0;
	char *end = NULL;

	float castval = strtof(args->argv[2], &end);
	HandleConversionErrors(args->argv[2], end);

	if (!Cvar_RegisterFloat(args->argv[1], castval, CVAR_NONE, ""))
		Log_Writef(LOG_ERROR, "Failed to set cvar: %s", args->argv[1]);
}

/*
* Function: Setb_Cmd
* Sets a cvar to a boolean value from the command line
* 
* 	args: The command arguments, argv[1] is the cvar name, argv[2] is the value
*/
static void Setb_Cmd(const cmdargs_t *args)
{
	if (args->argc != 3)
	{
		Log_Writef(LOG_INFO, "Usage: %s [cvarname] \"[value]\"", args->argv[0]);
		return;
	}

	errno = 0;
	char *end = NULL;

	bool castval = strtol(args->argv[2], &end, 10);
	HandleConversionErrors(args->argv[2], end);

	if (!Cvar_RegisterBool(args->argv[1], castval, CVAR_NONE, ""))
		Log_Writef(LOG_ERROR, "Failed to set cvar: %s", args->argv[1]);
}

/*
* Function: Cvar_Init
* Initializes the cvar system
* 
* Returns: A boolean if the initialization was successful or not
*/
bool Cvar_Init(void)
{
	if (initialized)
		return(true);

	Cmd_RegisterCommand("seta", Seta_Cmd, "Sets or registers a cvar to a string value");
	Cmd_RegisterCommand("seti", Seti_Cmd, "Sets or registers a cvar to an integer value");
	Cmd_RegisterCommand("setf", Setf_Cmd, "Sets or registers a cvar to a float value");
	Cmd_RegisterCommand("setb", Setb_Cmd, "Sets or registers a cvar to a boolean value");

	cvarmap = MemCache_Alloc(sizeof(*cvarmap));
	if (!cvarmap)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for cvar map");
		fclose(cvarfile);
		return(false);
	}

	cvarmap->capacity = DEF_CVAR_MAP_CAPACITY;
	cvarmap->numcvars = 0;

	cvarmap->cvars = MemCache_Alloc(sizeof(*cvarmap->cvars) * cvarmap->capacity);
	if (!cvarmap->cvars)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for cvar map entries");
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
			Log_Writef(LOG_ERROR, "Cvar file does not exist and cannot be recreated: %s", cvarfullname);
			return(false);
		}

		fclose(cvarfile);	// close the opened file if created
		cvarfile = NULL;
	}

	cvarfile = fopen(cvarfullname, "r");
	if (!cvarfile)
	{
		Log_Writef(LOG_ERROR, "Failed to open cvar file: %s", cvarfullname);
		return(false);
	}

	ReadCvarsFromFile(cvarfile, cvarfullname);

	fclose(cvarfile);
	cvarfile = NULL;

	// read the overrides file and populate the cvar list if cvars exist and if the file exists
	if (FileSys_FileExists(overridefilename))
	{
		FILE *overridesfile = fopen(overridefilename, "r");
		if (!overridesfile)
		{
			Log_Writef(LOG_ERROR, "Failed to open overrides file: %s", overridefilename);
			return(false);
		}

		Log_Writef(LOG_INFO, "Reading data from the overrides file: %s", overridefilename);

		ReadCvarsFromFile(overridesfile, overridefilename);

		fclose(overridesfile);
		overridesfile = NULL;
	}

	initialized = true;

	return(true);
}

/*
* Function: Cvar_Shutdown
* Shuts down the cvar system
*/
void Cvar_Shutdown(void)
{
	if (!initialized)
		return;

	Log_Write(LOG_INFO, "Shutting down cvar system");

#if defined(MENGINE_DEBUG)
	ListAllCvars();
#endif

	(void)ListAllCvars;

	cvarfile = fopen(cvarfullname, "w");
	if (!cvarfile)
	{
		Log_Writef(LOG_ERROR, "Failed to open cvar file: %s", cvarfullname);
		return;
	}

	// go through the map and write all the cvars to the file and free the memory
	for (size_t i=0; i<cvarmap->capacity; i++)
	{
		cvarentry_t *current = cvarmap->cvars[i];
		while (current)
		{
			cvar_t * const cvar = current->value;

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

/*
* Function: Cvar_Find
* Finds a cvar in the hashmap
* 
*	name: The name of the cvar
* 
* Returns: A pointer to the cvar if it exists or NULL
*/
cvar_t *Cvar_Find(const char *name)
{
	if (!name || !name[0])
		return(NULL);

	size_t index = HashCvarName(name);
	cvarentry_t *current = cvarmap->cvars[index];
	while (current)
	{
		if (strcmp(current->value->name, name) == 0)
			return(current->value);

		current = current->next;
	}

	return(NULL);
}

/*
* Function: Cvar_RegisterString
* Registers a string cvar
* 
*	name: The name of the cvar
*	value: The value of the cvar
*	flags: Bitfield to define the cvars properties
*	description: A short description of the cvar
* 
* Returns: A pointer to the newly registered cvar or NULL
*/
cvar_t *Cvar_RegisterString(const char *name, const char *value, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	snprintf(cvarvalue.s, sizeof(cvarvalue.s), "%s", value);
	return(RegisterCvar(name, cvarvalue, CVAR_STRING, flags, description));
}

/*
* Function: Cvar_RegisterInt
* Registers an integer cvar
* 
*	name: The name of the cvar
*	value: The value of the cvar
*	flags: Bitfield to define the cvars properties
*	description: A short description of the cvar
* 
* Returns: A pointer to the newly registered cvar or NULL
*/
cvar_t *Cvar_RegisterInt(const char *name, const int value, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	cvarvalue.i = value;
	return(RegisterCvar(name, cvarvalue, CVAR_INT, flags, description));
}

/*
* Function: Cvar_RegisterFloat
* Registers a float cvar
* 
*	name: The name of the cvar
*	value: The value of the cvar
*	flags: Bitfield to define the cvars properties
*	description: A short description of the cvar
* 
* Returns: A pointer to the newly registered cvar or NULL
*/
cvar_t *Cvar_RegisterFloat(const char *name, const float value, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	cvarvalue.f = value;
	return(RegisterCvar(name, cvarvalue, CVAR_FLOAT, flags, description));
}

/*
* Function: Cvar_RegisterBool
* Registers a boolean cvar
* 
*	name: The name of the cvar
*	value: The value of the cvar
*	flags: Bitfield to define the cvars properties
*	description: A short description of the cvar
* 
* Returns: A pointer to the newly registered cvar or NULL
*/
cvar_t *Cvar_RegisterBool(const char *name, const bool value, const unsigned long long flags, const char *description)
{
	cvarvalue_t cvarvalue = { 0 };
	cvarvalue.b = value;
	return(RegisterCvar(name, cvarvalue, CVAR_BOOL, flags, description));
}

/*
* Function: Cvar_GetString
* Gets the string value of a cvar
* 
*	cvar: The cvar to get the value from
*	out: The output buffer to store the value
* 
* Returns: A boolean if the operation was successful or not
*/
bool Cvar_GetString(const cvar_t *cvar, char *out)
{
	if ((!cvar) || (cvar->type != CVAR_STRING))
		return(false);

	snprintf(out, sizeof(cvar->value.s), "%s", cvar->value.s);
	return(true);
}

/*
* Function: Cvar_GetInt
* Gets the integer value of a cvar
* 
*	cvar: The cvar to get the value from
*	out: The output buffer to store the value
* 
* Returns: A boolean if the operation was successful or not
*/
bool Cvar_GetInt(const cvar_t *cvar, int *out)
{
	if ((!cvar) || (cvar->type != CVAR_INT))
		return(false);

	*out = cvar->value.i;
	return(true);
}

/*
* Function: Cvar_GetFloat
* Gets the float value of a cvar
* 
*	cvar: The cvar to get the value from
*	out: The output buffer to store the value
* 
* Returns: A boolean if the operation was successful or not
*/
bool Cvar_GetFloat(const cvar_t *cvar, float *out)
{
	if ((!cvar) || (cvar->type != CVAR_FLOAT))
		return(false);

	*out = cvar->value.f;
	return(true);
}

/*
* Function: Cvar_GetBool
* Gets the boolean value of a cvar
* 
*	cvar: The cvar to get the value from
*	out: The output buffer to store the value
* 
* Returns: A boolean if the operation was successful or not
*/
bool Cvar_GetBool(const cvar_t *cvar, bool *out)
{
	if ((!cvar) || (cvar->type != CVAR_BOOL))
		return(false);

	*out = cvar->value.b;
	return(true);
}

/*
* Function: Cvar_SetString
* Sets the string value of a cvar
* 
*	cvar: The cvar to set the value of
*	value: The value to set
*/
void Cvar_SetString(cvar_t *cvar, const char *value)
{
	if ((!cvar) || (cvar->type != CVAR_STRING) || (cvar->flags & CVAR_READONLY))
		return;

	snprintf(cvar->value.s, sizeof(cvar->value.s), "%s", value);
}

/*
* Function: Cvar_SetInt
* Sets the integer value of a cvar
* 
*	cvar: The cvar to set the value of
*	value: The value to set
*/
void Cvar_SetInt(cvar_t *cvar, const int value)
{
	if ((!cvar) || (cvar->type != CVAR_INT) || (cvar->flags & CVAR_READONLY))
		return;

	cvar->value.i = value;
}

/*
* Function: Cvar_SetFloat
* Sets the float value of a cvar
* 
*	cvar: The cvar to set the value of
*	value: The value to set
*/
void Cvar_SetFloat(cvar_t *cvar, const float value)
{
	if ((!cvar) || (cvar->type != CVAR_FLOAT) || (cvar->flags & CVAR_READONLY))
		return;

	cvar->value.f = value;
}

/*
* Function: Cvar_SetBool
* Sets the boolean value of a cvar
* 
*	cvar: The cvar to set the value of
*	value: The value to set
*/
void Cvar_SetBool(cvar_t *cvar, const bool value)
{
	if ((!cvar) || (cvar->type != CVAR_BOOL) || (cvar->flags & CVAR_READONLY))
		return;

	cvar->value.b = value;
}
