#!/bin/sh
#
# Rebuild Berryboot for Allwinner A10 devices
#

set -e

# Normal rPi build first
./rebuild-berryboot.sh

# Make uInitrd
# mkimage -A arm -T ramdisk -C none -n "uInitrd" -d buildroot-2012.05/output/images/rootfs.cpio.gz output/uInitrd
# edit: mkimage now done by rebuild-berryboot.sh

# Build kernel if it doesn't exist already
if [ ! -e output/kernel_a10_aufs.img ]; then
	echo Building A10 kernel
	if [ ! -e linux-allwinner-aufs34 ]; then
		git clone --depth=1 https://github.com/maxnet/linux-allwinner-aufs34
	fi
	cd linux-allwinner-aufs34
	make sun4i_aufs_defconfig ARCH=arm
	make -j4 uImage ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
	make -j4 modules ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
	make modules_install ARCH=arm INSTALL_MOD_PATH=output
	cd output
	# combine a10 modules with the ones in existing shared.tgz
	tar xzf ../../output/shared.tgz
	tar czf ../../output/shared.tgz *
	cd ../..
	cp linux-allwinner-aufs34/arch/arm/boot/uImage output/kernel_a10_aufs.img
fi

echo Build complete. Result is in \'output\' directory
echo Note: you need to manually build u-boot SPL for your specific device, and copy script.bin to the output directory
 
