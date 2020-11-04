#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int fd;
	char *file;
	char *file2;
	struct stat sb;
	char buf[80] = {0x00, };
	int flag = PROT_WRITE | PROT_READ;

	fd = open(argv[1], O_RDWR|O_CREAT);
	write(fd, "xxxx", 4);
	file = mmap(0,40,flag,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
	file2= mmap(0,40,flag,MAP_ANONYMOUS|MAP_PRIVATE,-1,41);
	file[0]='x';
	file[1]='x';
	file[2]='\0';
	file2[0]='y';
	file2[1]='y';
	file2[2]='\0';

	printf("%s\n", file);
	printf("%s\n", file2);

	munmap(file,40);
	close(fd);
}
