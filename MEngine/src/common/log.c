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
#define LOG_MSG_FMT "%s %s %s\n"
#define MAX_LOG_ENTRIES 256
#define MAX_LOG_FILES 5

typedef struct
{
	logtype_t type;
	time_t time;
	char msg[LOG_MAX_LEN];
	char *longmsg;
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

/*
* Function: CompareFileData
* Compares two filedata_t structs by their modification time
* 
*	a: The first filedata_t struct
*	b: The second filedata_t struct
* 
* Returns: The difference in time between the two times
*/
static int CompareFileData(const void *a, const void *b)
{
	return((int)difftime(((filedata_t *)b)->mtime, ((filedata_t *)a)->mtime));
}

/*
* Function: RemoveOldLogFiles
* Removes old log files from the logs directory based on the MAX_LOG_FILES define and the file filter
* 
*	dir: The directory to search for log files
* 
* Returns: A boolean if the operation was successful or not
*/
static bool RemoveOldLogFiles(const char *dir)
{
	const char *logfilter = "logs.*.log";
	unsigned int filecount = 0;

	filedata_t *filelist = FileSys_ListFiles(&filecount, dir, logfilter);

	if (filecount <= 0)
		return(true);		// return true if filecount is 0, next function will create log	files

	else if (!filelist)
		return(false);

	qsort(filelist, filecount, sizeof(*filelist), CompareFileData);

	while (filecount > MAX_LOG_FILES)
	{
		char oldfile[SYS_MAX_PATH] = { 0 };
		snprintf(oldfile, SYS_MAX_PATH, "%s", filelist[filecount - 1].filename);

		if (oldfile[0] != '\0' && remove(oldfile) == 0)
			filecount--;
	}

	FileSys_FreeFileList(filelist);
	return(true);
}

/*
* Function: GenLogFileName
* Generates a log file name based on the current date
* 
*	dir: The directory to store the log files
*	outfilename: The output filename
* 
* Returns: A boolean if the operation was successful or not
*/
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

/*
* Function: FormatSessionEntry
* Formats a session entry in the log file with the current date and time, log header
* 
*	logfullname: The full path to the log file
*/
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

/*
* Function: ProcessLogQueue
* Processes the log queue and writes the log entries to the log file in another thread
* 
*	args: The arguments to the thread function, unused for this function
*/
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

				if (entry->longmsg)
				{
					fprintf(logfile, LOG_MSG_FMT, timestr, logmsgtype[entry->type], entry->longmsg);
					MemCache_Free(entry->longmsg);
				}

				else
					fprintf(logfile, LOG_MSG_FMT, timestr, logmsgtype[entry->type], entry->msg);

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

			if (entry->longmsg)
			{
				fprintf(logfile, LOG_MSG_FMT, timestr, logmsgtype[entry->type], entry->longmsg);
				MemCache_Free(entry->longmsg);
			}

			else
				fprintf(logfile, LOG_MSG_FMT, timestr, logmsgtype[entry->type], entry->msg);

			fflush(logfile);

			logcount--;

			for (unsigned int i=0; i<logcount; i++)
				logqueue[i] = logqueue[i + 1];
		}

		Sys_UnlockMutex(loglock);
	}

	return(NULL);
}

/*
* Function: Log_Init
* Initializes the logging service
* 
* Returns: A boolean if initialization was successful or not
*/
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

	Log_Write(LOG_INFO, "Logs opened, logging started...");

	return(true);
}

/*
* Function: Log_Shutdown
* Shuts down the logging service
*/
void Log_Shutdown(void)
{
	if (!initialized)
		return;

	Log_Write(LOG_INFO, "Shutting down logging service...");

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

/*
* Function: Log_Write
* Writes a log message to the log queue for processing by the log thread
*
* 	type: The type of log message
* 	msg: The message to log, just a string
*/
void Log_Write(const logtype_t type, const char *msg)
{
	if ((logcount >= MAX_LOG_ENTRIES) || !initialized)
		return;

	Sys_LockMutex(loglock);

	logentry_t *entry = &logqueue[logcount];

	entry->time = time(NULL);
	entry->type = type;
	int len = snprintf(entry->msg, LOG_MAX_LEN, msg);

	if (len > LOG_MAX_LEN)
	{
		entry->longmsg = MemCache_Alloc(len + 1);
		if (entry->longmsg)
			snprintf(entry->longmsg, len + 1, msg);
	}

	logcount++;

	Sys_UnlockMutex(loglock);
	Sys_SignalCondVar(logcond);
}

/*
* Function: Log_Write
* Writes a formatted log message to the log queue for processing by the log thread
*
* 	type: The type of log message
* 	msg: The message to log, the log message format is the same as printf
*	...: The arguments to the format string
*/
void Log_Writef(const logtype_t type, const char *msg, ...)
{
	if ((logcount >= MAX_LOG_ENTRIES) || !initialized)
		return;

	Sys_LockMutex(loglock);

	logentry_t *entry = &logqueue[logcount];

	entry->time = time(NULL);
	entry->type = type;

	va_list arg;
	va_start(arg, msg);
	int len = vsnprintf(entry->msg, LOG_MAX_LEN, msg, arg);
	va_end(arg);

	if (len > LOG_MAX_LEN)
	{
		entry->longmsg = MemCache_Alloc(len + 1);
		if (entry->longmsg)
		{
			va_start(arg, msg);
			vsnprintf(entry->longmsg, len + 1, msg, arg);
			va_end(arg);
		}
	}

	logcount++;

	Sys_UnlockMutex(loglock);
	Sys_SignalCondVar(logcond);
}
