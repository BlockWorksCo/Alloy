


- Raspberry Pi 2
- Raspberry Pi 3


Make into a single processor kernel:
- "nosmp" flag in cmdline.txt
- no swap.

Limit RAM usage:
- add mem=128M

Stop /boot being modified.
- add noatime to /boot in /etc/fstab

Disable swap:
- sudo chmod -x /etc/init.d/dphys-swapfile
- sudo swapoff -a
- reboot.
- sudo rm /var/swap


Build Kernel:
- make ARCH=arm CROSS_COMPILE=/usr/bin/arm-linux-gnueabi- bcm2709_defconfig
- make ARCH=arm CROSS_COMPILE=/usr/bin/arm-linux-gnueabi- -j 6 zImage modules dtbs
- cd arch/arm/boot/
- linux/scripts/mkknlimg zImage ReactorKernel.img


