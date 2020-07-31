# Make Linux Guest Image

**This is only for qemu virt machine.**

Compile a vanilla Linux kernel:
```
wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.19.133.tar.xz
tar -xf linux-4.19.133.tar.xz
cd linux-4.19.133
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make defconfig
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make -j24
```

Copy `arch/arm64/boot/Image` to this folder.

Make a minial ramdisk and copy into this folder as `initrd.gz`.

Adjust `/chosen/linux,initrd-end` and make an all in one image:
```
make
```
