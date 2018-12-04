#!/usr/bin
cd rootfs
# cpio -idmv < ../rootfs.cpio
musl-gcc -static -o exp ../exp.c
find . | cpio -H newc -o > ../rootfs.cpio
