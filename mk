VFD=mtximage

clear

echo compiling .....
bcc -c -ansi t.c
as86 -o ts.o ts.s

echo linking .......
ld86 -o mtx -d ts.o t.o mtxlib /usr/lib/bcc/libc.a

mount -o loop $VFD /mnt
cp mtx /mnt/boot
umount /mnt

(cd USER; ./mku u1; ./mku u2)

rm *.o 
echo done

#qemu-system-x86_64 -fda mtximage -no-fd-bootchk -serial /dev/pts/12 -serial /dev/pts/2
# -serial mon:stdio
# -serial /dev/pts/12

qemu-system-x86_64 -fda mtximage -no-fd-bootchk -serial mon:stdio #-serial /dev/pts/2
