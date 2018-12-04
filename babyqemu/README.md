See the source code [here](https://github.com/qemu/qemu/blob/master/hw/misc/edu.c).

As you can see, the challenge removes the check of dma address. So just mmap a memory area for arbitray read and write.
The challenge doesn't remove the symbols, and you can see the structure for hitb device: HitbState.
```
00000000 HitbState       struc ; (sizeof=0x1BD0, align=0x10, copyof_1493)
00000000 pdev            PCIDevice_0 ?
000009F0 mmio            MemoryRegion_0 ?
00000AF0 thread          QemuThread_0 ?
00000AF8 thr_mutex       QemuMutex_0 ?
00000B20 thr_cond        QemuCond_0 ?
00000B50 stopping        db ?
00000B51                 db ? ; undefined
00000B52                 db ? ; undefined
00000B53                 db ? ; undefined
00000B54 addr4           dd ?
00000B58 fact            dd ?
00000B5C status          dd ?
00000B60 irq_status      dd ?
00000B64                 db ? ; undefined
00000B65                 db ? ; undefined
00000B66                 db ? ; undefined
00000B67                 db ? ; undefined
00000B68 dma             dma_state ?
00000B88 dma_timer       QEMUTimer_0 ?
00000BB8 dma_buf         db 4096 dup(?)
00001BB8 enc             dq ?                    ; offset
00001BC0 dma_mask        dq ?
00001BC8                 db ? ; undefined
00001BC9                 db ? ; undefined
00001BCA                 db ? ; undefined
00001BCB                 db ? ; undefined
00001BCC                 db ? ; undefined
00001BCD                 db ? ; undefined
00001BCE                 db ? ; undefined
00001BCF                 db ? ; undefined
00001BD0 HitbState       ends
```
There is a field called enc, and it's a function in binary. So to exploit the challenge, just leaking it and get the address of system@plt. 

Figure out 2 points:
the function cpu_physical_memory_rw:
```
void cpu_physical_memory_rw(target_phys_addr_t addr, uint8_t *buf, lint len, int is_write)

if (is_write)
{
	p = g2h(addr);
	memcpy(p, buf, len);
}
else
{
	p = g2h(addr);
	memcpy(buf, p, len);
}
```
You must pass a physical address(for guest) to it, so you have to transfer the virtual address into physical address first.

Another thing is it's hard to figure out how long the timer will be expired. 
```
timer_mod(
              (QEMUTimer_0 *)((char *)opaque + 2952),
              ((signed __int64)((unsigned __int128)(4835703278458516699LL * (signed __int128)v7) >> 64) >> 18)
            - (v7 >> 63)
            + 100);
```
So just sleep 1s~ I think it's long enough for the deivce driver.

The challenge is not so hard but interesting. If you don't understand what the device does, I suggest you to read some books of linux driver.

