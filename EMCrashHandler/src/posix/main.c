#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
	int fd = shm_open("/EMCrashHandlerFileMapping", O_CREAT | O_EXCL | O_RDWR, 0600);
	if (fd == -1)
	{
		perror("shm_open");
		return(1);
	}

	return(0);
}
