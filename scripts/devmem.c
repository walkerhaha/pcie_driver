#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int main(int argc, char **argv)
{
	int fd;
	void *map_base, *virt_addr;
	unsigned long long  reg;
	unsigned int value;
	int i = 0;
	switch (argc) {
	case 3:
		if (strcmp(argv[1], "-r")) {
			printf("usage:\nlinuxrw -r 0x10000000 linuxrw -w 0x10000000 0x20000000\n");
			exit(-1);
		}
		break;

	case 4:
		if (strcmp(argv[1], "-w")) {
			printf("usage:\nlinuxrw -r 0x10000000 linuxrw -w 0x10000000 0x20000000\n");
			exit(-1);
		}
		break;

	default :
		printf("usage:\nlinuxrw -r 0x10000000 linuxrw -w 0x10000000 0x20000000\n");
		exit (-1);
	}

	reg = strtol(argv[2], NULL, 16);
	printf("addr : 0x%llx\n", reg);

	if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) <0) {
		perror("open");
		return -1;
	}

	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, reg & ~MAP_MASK);

	virt_addr = map_base + (reg & MAP_MASK);
	if (argc == 3) {
		for (i = 0; i < 64; i++) {
			value = *((unsigned int *) virt_addr + i);
			printf("i:%-3d, value :0x%x\n", i, value);
		}
	} else if (argc == 4) {
		value = strtol(argv[3], NULL, 16);

		for (i = 0; i < 64; i++) {
			*((unsigned int *) virt_addr + i) = value;
			printf("i:%-3d value: 0x%x\n", i, *((unsigned int*)virt_addr + i));
		}
	}

	munmap(map_base,MAP_SIZE);

	close(fd);

	return 0;
}
