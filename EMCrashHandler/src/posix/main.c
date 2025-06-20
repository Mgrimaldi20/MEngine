#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../emstatus.h"

static void PrintError(const char *msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	errno = 0;
}

int main(int argc, char **argv)
{
	const char *logfilename = "crashstatus.log";
	char logfullpath[1024] = { 0 };

	snprintf(logfullpath, sizeof(logfullpath) - 1, "%s", logfilename);
	logfullpath[sizeof(logfullpath) - 1] = '\0';

	if (argc > 1)
		snprintf(logfullpath, sizeof(logfullpath) - 1, "%s/%s", argv[1], logfilename);

	FILE *logfile = fopen(logfullpath, "w+");
	if (!logfile)
	{
		fprintf(stderr, "Cannot open log file: %s, directory might not exist\n", logfullpath);
		return(1);
	}

	fprintf(logfile, "Starting crash handler\n");
	fprintf(logfile, "Opening logs at: %s\n", logfullpath);

	int fd = shm_open("/EMCrashHandlerFileMapping", O_RDWR, 0);
	if (fd == -1)
	{
		PrintError("Error opening the shared memory region");
		fclose(logfile);
		return(1);
	}

	emstatus_t *emstatus = mmap(NULL, sizeof(emstatus_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (emstatus == MAP_FAILED || !emstatus)
	{
		PrintError("Error mapping the shared memory file");
		close(fd);
		fclose(logfile);
		return(1);
	}

	while (1)
	{
		if (emstatus->userdata[0] != '\0')
		{
			fprintf(logfile, "Writing User Data:\n\t%s\n", emstatus->userdata);
			fflush(logfile);
		}

		if (emstatus->status == EMSTATUS_EXIT_OK)
		{
			fprintf(logfile, "No errors occurred during engine runtime, exiting successfully\n");
			fflush(logfile);
			break;
		}

		if (emstatus->status == EMSTATUS_EXIT_ERROR)
		{
			fprintf(logfile, "The engine has exited due to a known error, please check the main engine logs for information\n");
			fflush(logfile);
			break;
		}

		if (emstatus->status != EMSTATUS_EXIT_CRASH)
		{
			fprintf(logfile, "[EMSTATUS_ERROR] | An error has occurred during engine runtime, please check file[%s] for stack trace\n", logfullpath);

			for (int i=0; i<EMCH_MAX_FRAMES; i++)
			{
				if (emstatus->stacktrace[i][0] != '\0')
				{
					fprintf(logfile, "[EMSTATUS_ERROR] | %s\n", emstatus->stacktrace[i]);
					fflush(logfile);
				}
			}

			break;
		}
	}

	if (munmap(emstatus, sizeof(emstatus_t)) != 0)
		PrintError("Failed to unmap memory, you may wish to check ipcs to clean it up manually");

	close(fd);
	fclose(logfile);

	return(0);
}
