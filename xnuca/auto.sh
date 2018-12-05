#!/usr/bin

mkdir rootfs
cd rootfs
cpio -idmv < ../rootfs.img

musl-gcc -static -o exp ../exp.c
find . | cpio -H newc -o > ../rootfs.img