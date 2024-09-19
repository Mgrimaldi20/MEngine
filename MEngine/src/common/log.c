#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <threads.h>
#include "common.h"

#define LOG_TIMESTR_LEN 100
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
static mtx_t loglock;
static cnd_t logcond;
static thrd_t logthread;
static volatile bool stopthreads;
static unsigned int logcount;
static logentry_t logqueue[MAX_LOG_ENTRIES];

static int CompareFileData(const void *a, const void *b)
{
	return((int)difftime(((sysfiledata_t *)b)->lastwritetime, ((sysfiledata_t *)a)->lastwritetime));
}

static bool RemoveOldLogFiles(const char *dir)	// why does this code exist? why did I create it? its horrible :)
{
	const char *logfilter = "logs.*.log";

	unsigned int filecount = Sys_CountFiles(dir, logfilter);
	if (filecount <= 0)
		return(true);	// just return true here too, the next func will create logs

	sysfiledata_t *filelist = MemCache_Alloc(sizeof(*filelist) * filecount);
	if (!filelist)
		return(false);

	if (!Sys_ListFiles(dir, logfilter, filelist, filecount))
	{
		MemCache_Free(filelist);
		return(false);
	}

	qsort(filelist, filecount, sizeof(*filelist), CompareFileData);

	unsigned int tempfilecount = filecount;

	while (tempfilecount > MAX_LOG_FILES)
	{
		char oldfile[SYS_MAX_PATH] = { 0 };
		snprintf(oldfile, SYS_MAX_PATH, "%s", filelist[--tempfilecount].filename);

		if (oldfile[0] != '\0')
			remove(oldfile);
	}

	MemCache_Free(filelist);
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

static int ProcessLogQueue(void *args)
{
	(void)args;

	while (1)
	{
		mtx_lock(&loglock);

		while (logcount == 0 && !stopthreads)
			cnd_wait(&logcond, &loglock);

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
				fflush(logfile);
			}

			logcount = 0;

			mtx_unlock(&loglock);
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

		mtx_unlock(&loglock);
	}

	return(0);
}

bool Log_Init(void)
{
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

	if (mtx_init(&loglock, mtx_plain) != thrd_success ||
		cnd_init(&logcond) != thrd_success ||
		thrd_create(&logthread, ProcessLogQueue, NULL) != thrd_success)
	{
		thrd_join(logthread, NULL);
		cnd_destroy(&logcond);
		mtx_destroy(&loglock);

		fclose(logfile);
		logfile = NULL;

		return(false);
	}

	Log_WriteSeq(LOG_INFO, "Logs opened, logging started...");
	return(true);
}

void Log_Shutdown(void)
{
	Log_WriteSeq(LOG_INFO, "Shutting down logging service...");

	Sys_Sleep(1000);	// wait for log thread to finish, make sure all logs are written, not really required

	stopthreads = true;

	cnd_signal(&logcond);

	thrd_join(logthread, NULL);
	cnd_destroy(&logcond);
	mtx_destroy(&loglock);

	if (logfile)
	{
		fflush(logfile);
		fclose(logfile);
		logfile = NULL;
	}
}

void Log_Write(logtype_t type, const char *msg, ...)
{
	if (logcount >= MAX_LOG_ENTRIES)
		return;

	mtx_lock(&loglock);

	logentry_t *entry = &logqueue[logcount];

	entry->time = time(NULL);
	entry->type = type;

	va_list arg;
	va_start(arg, msg);
	vsnprintf(entry->msg, LOG_MAX_LEN, msg, arg);
	va_end(arg);

	logcount++;

	mtx_unlock(&loglock);

	cnd_signal(&logcond);
}

void Log_WriteSeq(logtype_t type, const char *msg, ...)
{
	mtx_lock(&loglock);

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
	fflush(logfile);

	mtx_unlock(&loglock);
}

void Log_WriteLargeSeq(logtype_t type, const char *msg, ...)
{
	mtx_lock(&loglock);

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
		fflush(logfile);

		MemCache_Free(logmsg);
	}

	mtx_unlock(&loglock);
}
