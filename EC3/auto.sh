#!/bin/sh

musl-gcc -static -o initramfs-busybox-x86_64/exp exp.c
cd initramfs-busybox-x86_64
find . | cpio -H newc -o > ../initramfs-busybox-x86_64.cpio
cd ..
gzip initramfs-busybox-x86_64.cpio
