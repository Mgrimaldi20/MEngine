#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include "sys/sys.h"
#include "common.h"

#define LOG_TIMESTR_LEN 32
#define LOG_TIME_FMT "%Y-%m-%d %H:%M:%S"
#define MAX_LOG_ENTRIES 256
#define MAX_LOG_FILES 5

typedef struct
{
	logtype_t type;
	time_t time;
	char msg[LOG_MAX_LEN];
} logentry_t;

static const char *logmsgtype[] =
{
	"[INFO]",
	"[WARN]",
	"[ERROR]"
};

static FILE *logfile;
static mutex_t *loglock;
static condvar_t *logcond;
static thread_t *logthread;
static volatile bool stopthreads;
static unsigned int logcount;
static logentry_t logqueue[MAX_LOG_ENTRIES];

static bool initialized;

static int CompareFileData(const void *a, const void *b)
{
	return((int)difftime(((filedata_t *)b)->mtime, ((filedata_t *)a)->mtime));
}

static bool RemoveOldLogFiles(const char *dir)	// ahhh its alright
{
	const char *logfilter = "logs.*.log";
	unsigned int filecount = 0;

	filedata_t *filelist = FileSys_ListFiles(&filecount, dir, logfilter);

	if (filecount <= 0)
		return(true);		// return true if filecount is 0, next function will create log	files

	else if (!filelist)
		return(false);

	qsort(filelist, filecount, sizeof(*filelist), CompareFileData);

	unsigned int tempfilecount = filecount;

	while (tempfilecount > MAX_LOG_FILES)
	{
		char oldfile[SYS_MAX_PATH] = { 0 };
		snprintf(oldfile, SYS_MAX_PATH, "%s", filelist[--tempfilecount].filename);

		if (oldfile[0] != '\0')
			remove(oldfile);
	}

	FileSys_FreeFileList(filelist);
	return(true);
}

static bool GenLogFileName(const char *dir, char *outfilename)
{
	if (!dir || !outfilename)
		return(false);

	struct tm timeinfo;
	char timename[LOG_TIMESTR_LEN] = { 0 };

	time_t rawtime = time(NULL);
	Sys_Localtime(&timeinfo, &rawtime);
	strftime(timename, LOG_TIMESTR_LEN, "%Y%m%d", &timeinfo);

	snprintf(outfilename, SYS_MAX_PATH, "%s/logs.%s.log", dir, timename);
	return(true);
}

static void FormatSessionEntry(const char *logfullname)
{
	FILE *temp = fopen(logfullname, "r+");
	if (temp)
	{
		struct tm timeinfo;
		char timestr[LOG_TIMESTR_LEN] = { 0 };

		time_t rawtime = time(NULL);
		Sys_Localtime(&timeinfo, &rawtime);
		strftime(timestr, LOG_TIMESTR_LEN, LOG_TIME_FMT, &timeinfo);

		fseek(temp, 0, SEEK_END);

		fprintf(temp, "\n\n%s\n%s %s\n%s\n",
			"--------------------------------------",
			"New run started at", timestr,
			"--------------------------------------"
		);

		fclose(temp);
	}
}

static void *ProcessLogQueue(void *args)
{
	(void)args;

	while (1)
	{
		Sys_LockMutex(loglock);

		while (logcount == 0 && !stopthreads)
			Sys_WaitCondVar(logcond, loglock);

		if (stopthreads)
		{
			for (unsigned int i=0; i<logcount; i++)	// write remaining log entries to file
			{
				logentry_t *entry = &logqueue[i];

				struct tm timeinfo;
				char timestr[LOG_TIMESTR_LEN] = { 0 };

				Sys_Localtime(&timeinfo, &entry->time);
				strftime(timestr, LOG_TIMESTR_LEN, LOG_TIME_FMT, &timeinfo);

				fprintf(logfile, "%s %s %s\n", timestr, logmsgtype[entry->type], entry->msg);

				if (entry->type == LOG_ERROR)
					fflush(logfile);
			}

			logcount = 0;

			Sys_UnlockMutex(loglock);
			break;
		}

		logentry_t *entry = &logqueue[0];

		if (logcount > 0)
		{
			struct tm timeinfo;
			char timestr[LOG_TIMESTR_LEN] = { 0 };

			Sys_Localtime(&timeinfo, &entry->time);
			strftime(timestr, LOG_TIMESTR_LEN, LOG_TIME_FMT, &timeinfo);

			fprintf(logfile, "%s %s %s\n", timestr, logmsgtype[entry->type], entry->msg);
			fflush(logfile);

			logcount--;

			for (unsigned int i=0; i<logcount; i++)
				logqueue[i] = logqueue[i + 1];
		}

		Sys_UnlockMutex(loglock);
	}

	return(NULL);
}

bool Log_Init(void)
{
	if (initialized)
		return(true);

	const char *logdir = "logs";

	if (!Sys_Mkdir(logdir))
		return(false);

	if (!RemoveOldLogFiles(logdir))
		return(false);

	char logfullname[SYS_MAX_PATH] = { 0 };
	if (!GenLogFileName(logdir, logfullname))
		return(false);

	FormatSessionEntry(logfullname);

	logfile = fopen(logfullname, "a");
	if (!logfile)
		return(false);

	stopthreads = false;

	loglock = Sys_CreateMutex();
	logcond = Sys_CreateCondVar();
	logthread = Sys_CreateThread(ProcessLogQueue, NULL);

	if (!loglock || !logcond || !logthread)
	{
		Sys_JoinThread(logthread);
		Sys_DestroyCondVar(logcond);
		Sys_DestroyMutex(loglock);

		logthread = NULL;
		logcond = NULL;
		loglock = NULL;

		fclose(logfile);
		logfile = NULL;

		return(false);
	}

	initialized = true;

	Log_WriteSeq(LOG_INFO, "Logs opened, logging started...");

	return(true);
}

void Log_Shutdown(void)
{
	if (!initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down logging service...");

	Sys_Sleep(1000);	// wait for log thread to finish, make sure all logs are written, not really required

	stopthreads = true;

	Sys_SignalCondVar(logcond);

	if (loglock || logcond || logthread)
	{
		Sys_JoinThread(logthread);
		Sys_DestroyCondVar(logcond);
		Sys_DestroyMutex(loglock);

		logthread = NULL;
		logcond = NULL;
		loglock = NULL;
	}

	if (logfile)
	{
		fflush(logfile);
		fclose(logfile);
		logfile = NULL;
	}

	initialized = false;
}

void Log_Write(const logtype_t type, const char *msg, ...)
{
	if ((logcount >= MAX_LOG_ENTRIES) || (!initialized))
		return;

	Sys_LockMutex(loglock);

	logentry_t *entry = &logqueue[logcount];

	entry->time = time(NULL);
	entry->type = type;

	va_list arg;
	va_start(arg, msg);
	vsnprintf(entry->msg, LOG_MAX_LEN, msg, arg);
	va_end(arg);

	logcount++;

	Sys_UnlockMutex(loglock);

	Sys_SignalCondVar(logcond);
}

void Log_WriteSeq(const logtype_t type, const char *msg, ...)
{
	if (!initialized)
		return;

	Sys_LockMutex(loglock);

	struct tm timeinfo;
	char timestr[LOG_TIMESTR_LEN] = { 0 };
	char logmsg[LOG_MAX_LEN] = { 0 };

	time_t timer = time(NULL);
	Sys_Localtime(&timeinfo, &timer);
	strftime(timestr, LOG_TIMESTR_LEN, LOG_TIME_FMT, &timeinfo);

	va_list arg;
	va_start(arg, msg);
	vsnprintf(logmsg, LOG_MAX_LEN, msg, arg);
	va_end(arg);

	fprintf(logfile, "%s %s %s\n", timestr, logmsgtype[type], logmsg);

	if (type == LOG_ERROR)
		fflush(logfile);

	Sys_UnlockMutex(loglock);
}

void Log_WriteLargeSeq(const logtype_t type, const char *msg, ...)
{
	if (!initialized)
		return;

	Sys_LockMutex(loglock);

	va_list arg;
	va_start(arg, msg);
	int len = vsnprintf(NULL, 0, msg, arg);
	va_end(arg);

	char *logmsg = MemCache_Alloc(len + 1);
	if (logmsg)
	{
		va_start(arg, msg);
		vsnprintf(logmsg, len + 1, msg, arg);
		va_end(arg);

		struct tm timeinfo;
		char timestr[LOG_TIMESTR_LEN] = { 0 };

		time_t timer = time(NULL);
		Sys_Localtime(&timeinfo, &timer);
		strftime(timestr, LOG_TIMESTR_LEN, LOG_TIME_FMT, &timeinfo);

		fprintf(logfile, "%s %s %s\n", timestr, logmsgtype[type], logmsg);

		if (type == LOG_ERROR)
			fflush(logfile);

		MemCache_Free(logmsg);
	}

	Sys_UnlockMutex(loglock);
}
