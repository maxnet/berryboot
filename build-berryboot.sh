#!/bin/sh

set -e
cd buildroot-2015.02

if [ -n "$1" ]; then
	if [ ! -e "../configs/$1" ]; then
		echo configs/$1 does not exist
		exit 1
	fi

	support/kconfig/merge_config.sh -m ../configs/berryboot_defconfig ../configs/$1
	make olddefconfig
fi

if [ ! -e .config ]; then
	echo No target device selected yet
	echo
	echo Run: $0 \<device\>
	echo -n "Supported devices: "
	(cd ../configs ; ls device*)
	echo
	exit 1
fi

# Let buildroot build everything
make

cd ..
cp -n LICENSE.berryboot output
cp -f buildroot-2015.02/output/images/rootfs.cpio.uboot output/berryboot.img
cp -f buildroot-2015.02/output/images/kernel*.img buildroot-2015.02/output/images/shared.tgz output || true

if [ -e buildroot-2015.02/output/images/rpi-firmware ]; then
	cp -rf buildroot-2015.02/output/images/rpi-firmware/* output
fi

# Pi kernel requires special treatment. Need to run mkknlimg on it, so bootloader knows it has DTB support
if [ -e buildroot-2015.02/output/images/kernel_rpi_aufs.img ]; then
	buildroot-2015.02/output/host/usr/bin/mkknlimg buildroot-2015.02/output/images/kernel_rpi_aufs.img output/kernel_rpi_aufs.img
fi
if [ -e buildroot-2015.02/output/images/kernel_rpi2_aufs.img ]; then
	buildroot-2015.02/output/host/usr/bin/mkknlimg buildroot-2015.02/output/images/kernel_rpi2_aufs.img output/kernel_rpi2_aufs.img
fi

echo
echo Build finished.
echo Copy contents of "output" folder to SD card.
echo
