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

	if (ftruncate(fd, sizeof(emstatus_t)) == -1)
	{
		perror("ftruncate");
		close(fd);
		return(1);
	}

	emstatus_t *emstatus = mmap(NULL, sizeof(emstatus_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (emstatus == MAP_FAILED)
	{
		perror("mmap");
		close(fd);
		return(1);
	}

	while (1)
	{
	}

	return(0);
}
