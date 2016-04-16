 ///
 /// @file    demo.c
 /// @author  double(aliceston@163.com)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


#include <sys/mman.h>

int main()
{
	int fd;

	fd = open("m.txt", O_RDWR);

	if(fd == -1)
	{

			perror("fd ");
			return -1;
	}

	char *addr = mmap(NULL, 1<<10, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 4096);
	if(addr == NULL || addr ==(void *)-1)
	{
		perror("mmp");
		return -1;
	}


		munmap(addr, 7777);
	return 0;
}
