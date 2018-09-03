#!/bin/sh

set -e

BUILDROOT=buildroot-2018.08
EXTERNAL="$PWD/external"

cd $BUILDROOT

if [ -n "$1" ]; then
	if [ ! -e "../configs/$1" ]; then
		echo configs/$1 does not exist
		exit 1
	fi

	support/kconfig/merge_config.sh -m ../configs/berryboot_defconfig ../configs/$1
	make BR2_EXTERNAL=$EXTERNAL olddefconfig
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
make BR2_EXTERNAL=$EXTERNAL

cd ..
cp -n LICENSE.berryboot output
cp -f $BUILDROOT/output/images/rootfs.cpio.uboot output/berryboot.img
cp -f $BUILDROOT/output/images/kernel*.img $BUILDROOT/output/images/shared.tgz output || true

if [ -e $BUILDROOT/output/images/rpi-firmware ]; then
	cp -rf $BUILDROOT/output/images/rpi-firmware/* output
	cp -rf $BUILDROOT/output/images/bcm27*.dtb output
	cp -rf $BUILDROOT/output/images/overlays output
	rm -f output/start_db.elf output/fixup_db.dat
fi

echo
echo Build finished.
echo Copy contents of "output" folder to SD card.
echo
