#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"

#define CFG_BUFFER_SIZE 4096

static unsigned int Hash(const char *key, size_t keylen)
{
	unsigned int hash = 0;
	for (size_t i=0; i<keylen; i++)
		hash = (hash << 5) + key[i];

	return(hash % CFG_TABLE_SIZE);
}

static cfg_t *CreateEntry(const char *key, const char *value)
{
	size_t keylen = strlen(key);
	size_t valuelen = strlen(value);

	cfg_t *entry = malloc(sizeof(*entry) + keylen + 1 + valuelen + 1);
	if (!entry)
		return(NULL);

	entry->key = (char *)(entry + 1);
	entry->value = entry->key + keylen + 1;
	entry->keylen = keylen;
	entry->valuelen = valuelen;

	memcpy(entry->key, key, keylen + 1);
	memcpy(entry->value, value, valuelen + 1);

	entry->next = NULL;

	return(entry);
}

static bool ParseLine(cfg_t *cfg, char *line)
{
	char *saveptr = NULL;

	char *trimmedline = Sys_Strtok(line, "\n", &saveptr);
	if (!trimmedline)
		return(true);

	while ((*trimmedline == ' ') || (*trimmedline == '\t'))
		trimmedline++;

	if (*trimmedline == ';')
		return(true);

	char *key = Sys_Strtok(trimmedline, "=", &saveptr);
	char *value = Sys_Strtok(NULL, " ", &saveptr);

	if (key && value)
	{
		cfg_t *newentry = CreateEntry(key, value);
		if (!newentry)
		{
			Log_WriteSeq(LOG_ERROR, "%s: Failed to create an entry into the config hash map", __func__);
			return(false);
		}

		unsigned int index = Hash(key, newentry->keylen);

		newentry->next = cfg->table[index];
		cfg->table[index] = newentry;
	}

	return(true);
}

static void WriteToFile(cfg_t *cfg)
{
	char *tempfname = "tempcfg.tmp";

	FILE *tempfile = fopen(tempfname, "w");
	if (!tempfile)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Failed to create temporary file: %s", __func__, strerror(errno));
		return;
	}

	FILE *file = fopen(cfg->filename, "r+");
	if (!file)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Failed to open file: %s", __func__, strerror(errno));
		fclose(tempfile);
		return;
	}

	char buffer[CFG_BUFFER_SIZE];
	cfg_t *entry = NULL;

	while (fgets(buffer, CFG_BUFFER_SIZE, file))
	{
		bool lineprocessed = false;

		for (int i=0; i<CFG_TABLE_SIZE; i++)
		{
			entry = cfg->table[i];
			while (entry)
			{
				if (entry->key && strstr(buffer, entry->key))
				{
					fprintf(tempfile, "%s=%s\n", entry->key, entry->value);
					lineprocessed = true;
					break;
				}

				entry = entry->next;
			}

			if (lineprocessed)
				break;
		}

		if (!lineprocessed)
			fprintf(tempfile, "%s", buffer);
	}

	fclose(file);
	fclose(tempfile);

	if (remove(cfg->filename) != 0)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Failed to remove original config file", __func__);
		return;
	}

	if (rename(tempfname, cfg->filename) != 0)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Failed to remove temp config file", __func__);
		return;
	}
}

static bool HandleConversionErrors(cfg_t *cfg, char *data, char *endptr)	// this level of checking should do
{
	if (data == endptr)
	{
		Log_Write(LOG_WARN, "%s: No digits found, 0 returned", cfg->filename);
		return(false);
	}

	if (errno == ERANGE)
	{
		Log_Write(LOG_WARN, "%s: Number returned out of range: %s", cfg->filename, strerror(errno));
		return(false);
	}

	if ((errno == 0) && endptr && (*endptr != 0))
	{
		Log_Write(LOG_WARN, "%s: Number returned is valid but additional characters remain", cfg->filename);
		return(false);
	}

	if ((errno != 0))
	{
		Log_Write(LOG_WARN, "%s: Unspecified error occurred: %s", cfg->filename, strerror(errno));
		return(false);
	}

	return(true);
}

cfg_t *Cfg_Init(const char *filename)
{
	cfg_t *cfg = malloc(sizeof(*cfg));
	if (!cfg)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Unable to create config hash map", __func__);
		return(NULL);
	}

	for (int i=0; i<CFG_TABLE_SIZE; i++)
		cfg->table[i] = NULL;

	char fullpath[SYS_MAX_PATH] = { 0 };
	snprintf(fullpath, SYS_MAX_PATH, "configs/%s", filename);	// just search in the configs directory

	FILE *file = fopen(fullpath, "r");
	if (!file)
	{
		Log_WriteSeq(LOG_ERROR, "Unable to open file: %s, %s", fullpath, strerror(errno));
		free(cfg);
		return(NULL);
	}

	snprintf(cfg->filename, SYS_MAX_PATH, "%s", fullpath);

	char buffer[CFG_BUFFER_SIZE];
	while (fgets(buffer, CFG_BUFFER_SIZE, file))
	{
		if (!ParseLine(cfg, buffer))
		{
			fclose(file);
			Cfg_Shutdown(cfg);
			return(NULL);
		}
	}

	fclose(file);
	return(cfg);
}

void Cfg_Shutdown(cfg_t *cfg)
{
	if (!cfg)
		return;

	WriteToFile(cfg);

	cfg_t *entry = NULL;
	cfg_t *temp = NULL;

	for (int i=0; i<CFG_TABLE_SIZE; i++)
	{
		entry = cfg->table[i];

		while (entry)
		{
			temp = NULL;

			if (!entry)
				continue;

			temp = entry;
			entry = entry->next;
			free(temp);	// should only have to free the single entry now
		}
	}

	free(cfg);
	cfg = NULL;
}


// I kinda really hate these set of functions, they really suck and are extremely ugly
char *Cfg_GetStr(cfg_t *cfg, const char *key)
{
	if (!cfg)
	{
		Log_Write(LOG_ERROR, "Config file has not been initialised yet, need to call Cfg_Init to create a cfg before using");
		return(NULL);
	}

	unsigned int index = Hash(key, strlen(key));
	cfg_t *entry = cfg->table[index];

	while (entry)
	{
		if (strcmp(entry->key, key) == 0)
			return(entry->value);

		entry = entry->next;
	}

	Log_Write(LOG_WARN, "%s: Key: '%s' not found", cfg->filename, key);
	return("");
}

long Cfg_GetNum(cfg_t *cfg, const char *key)
{
	char *endptr = NULL;
	char *data = Cfg_GetStr(cfg, key);

	errno = 0;
	long ret = strtol(data, &endptr, 10);

	if (!HandleConversionErrors(cfg, data, endptr))
		return(0);

	return(ret);
}

unsigned long Cfg_GetUNum(cfg_t *cfg, const char *key)
{
	char *endptr = NULL;
	char *data = Cfg_GetStr(cfg, key);

	errno = 0;
	unsigned long ret = strtoul(data, &endptr, 10);

	if(!HandleConversionErrors(cfg, data, endptr))
		return(0);

	return(ret);
}

long long Cfg_GetLNum(cfg_t *cfg, const char *key)
{
	char *endptr = NULL;
	char *data = Cfg_GetStr(cfg, key);

	errno = 0;
	long long ret = strtoll(data, &endptr, 10);

	if(!HandleConversionErrors(cfg, data, endptr))
		return(0);

	return(ret);
}

unsigned long long Cfg_GetULNum(cfg_t *cfg, const char *key)
{
	char *endptr = NULL;
	char *data = Cfg_GetStr(cfg, key);

	errno = 0;
	unsigned long long ret = strtoull(data, &endptr, 10);

	if(!HandleConversionErrors(cfg, data, endptr))
		return(0);

	return(ret);
}

float Cfg_GetFloat(cfg_t *cfg, const char *key)
{
	char *endptr = NULL;
	char *data = Cfg_GetStr(cfg, key);

	errno = 0;
	float ret = strtof(data, &endptr);

	if(!HandleConversionErrors(cfg, data, endptr))
		return(0.0f);

	return(ret);
}

double Cfg_GetDouble(cfg_t *cfg, const char *key)
{
	char *endptr = NULL;
	char *data = Cfg_GetStr(cfg, key);

	errno = 0;
	double ret = strtod(data, &endptr);

	if(!HandleConversionErrors(cfg, data, endptr))
		return(0.0);

	return(ret);
}

long double Cfg_GetLDouble(cfg_t *cfg, const char *key)
{
	char *endptr = NULL;
	char *data = Cfg_GetStr(cfg, key);

	errno = 0;
	long double ret = strtold(data, &endptr);

	if(!HandleConversionErrors(cfg, data, endptr))
		return(0.0L);

	return(ret);
}

bool Cfg_SetStr(cfg_t *cfg, const char *key, const char *val)
{
	if (!cfg)
	{
		Log_Write(LOG_ERROR, "Config file has not been initialised yet, need to call Cfg_Init to create a cfg before using");
		return(false);
	}

	unsigned int index = Hash(key, strlen(key));
	size_t valuelen = strlen(val);
	cfg_t *entry = cfg->table[index];

	while (entry)
	{
		if (strcmp(entry->key, key) != 0)
			entry = entry->next;

		free(entry->value);

		entry->value = malloc(valuelen + 1);
		if (!entry->value)
		{
			Log_Write(LOG_ERROR, "%s: Unable to allocate memory to place the new value", cfg->filename);
			return(false);
		}

		memcpy(entry->value, val, valuelen + 1);
		entry->valuelen = valuelen;

		return(true);
	}

	Log_Write(LOG_WARN, "%s: Key: '%s' not found", cfg->filename, key);
	return(false);
}

bool Cfg_SetNum(cfg_t *cfg, const char *key, const long val)
{
	char data[CFG_BUFFER_SIZE] = { 0 };

	if (snprintf(data, CFG_BUFFER_SIZE, "%ld", val) < 0)
	{
		Log_Write(LOG_ERROR, "%s: Encoding error, could not convert %ld into a string", cfg->filename, val);
		return(false);
	}

	if (!Cfg_SetStr(cfg, key, data))
		return(false);

	return(true);
}

bool Cfg_SetUNum(cfg_t *cfg, const char *key, const unsigned long val)
{
	char data[CFG_BUFFER_SIZE] = { 0 };

	if (snprintf(data, CFG_BUFFER_SIZE, "%lu", val) < 0)
	{
		Log_Write(LOG_ERROR, "%s: Encoding error, could not convert %lu into a string", cfg->filename, val);
		return(false);
	}

	if (!Cfg_SetStr(cfg, key, data))
		return(false);

	return(true);
}

bool Cfg_SetLNum(cfg_t *cfg, const char *key, const long long val)
{
	char data[CFG_BUFFER_SIZE] = { 0 };

	if (snprintf(data, CFG_BUFFER_SIZE, "%lld", val) < 0)
	{
		Log_Write(LOG_ERROR, "%s: Encoding error, could not convert %lld into a string", cfg->filename, val);
		return(false);
	}

	if (!Cfg_SetStr(cfg, key, data))
		return(false);

	return(true);
}

bool Cfg_SetULNum(cfg_t *cfg, const char *key, const unsigned long long val)
{
	char data[CFG_BUFFER_SIZE] = { 0 };

	if (snprintf(data, CFG_BUFFER_SIZE, "%llu", val) < 0)
	{
		Log_Write(LOG_ERROR, "%s: Encoding error, could not convert %llu into a string", cfg->filename, val);
		return(false);
	}

	if (!Cfg_SetStr(cfg, key, data))
		return(false);

	return(true);
}

bool Cfg_SetFloat(cfg_t *cfg, const char *key, const float val)
{
	char data[CFG_BUFFER_SIZE] = { 0 };

	if (snprintf(data, CFG_BUFFER_SIZE, "%f", val) < 0)
	{
		Log_Write(LOG_ERROR, "%s: Encoding error, could not convert %f into a string", cfg->filename, val);
		return(false);
	}

	if (!Cfg_SetStr(cfg, key, data))
		return(false);

	return(true);
}

bool Cfg_SetDouble(cfg_t *cfg, const char *key, const double val)
{
	char data[CFG_BUFFER_SIZE] = { 0 };

	if (snprintf(data, CFG_BUFFER_SIZE, "%lf", val) < 0)
	{
		Log_Write(LOG_ERROR, "%s: Encoding error, could not convert %lf into a string", cfg->filename, val);
		return(false);
	}

	if (!Cfg_SetStr(cfg, key, data))
		return(false);

	return(true);
}

bool Cfg_SetLDouble(cfg_t *cfg, const char *key, const long double val)
{
	char data[CFG_BUFFER_SIZE] = { 0 };

	if (snprintf(data, CFG_BUFFER_SIZE, "%Lf", val) < 0)
	{
		Log_Write(LOG_ERROR, "%s: Encoding error, could not convert %Lf into a string", cfg->filename, val);
		return(false);
	}

	if (!Cfg_SetStr(cfg, key, data))
		return(false);

	return(true);
}
