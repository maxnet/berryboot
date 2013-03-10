#!/bin/sh

set -e

if [ ! -e output/LICENSE.berryboot ]; then
	cp -f LICENSE.berryboot output
fi

cd buildroot-2012.05

# Delete BerrybootUI/installer build directory to force rebuild
if [ -e output/build ]; then
	rm -rf output/build/berryboot* || true 
fi

# Let buildroot build everything
make

# Copy the files we are interested in to the toplevel 'output' directory
cp output/images/shared.tgz ../output
#cp output/images/zImage ../output/kernel_berryboot.img
cp output/images/zImage ../output/kernel_rpi_aufs.img
#cp output/images/rootfs.cpio.gz ../output/berryboot.img
output/host/usr/bin/mkimage -A arm -T ramdisk -C none -n "uInitrd" -d output/images/rootfs.cpio.gz ../output/berryboot.img

cd ..
if [ ! -e output/start.elf ]; then
	echo "Downloading Raspberry Pi firmware"
	git clone --depth 1 git://github.com/raspberrypi/firmware.git
	cp firmware/boot/start*.elf output
	cp firmware/boot/fixup*.dat output
	cp firmware/boot/bootcode.bin output
fi

# add /opt/vc binaries and libraries to shared.tgz. Leaving out examples
cd firmware/hardfp
rm -rf opt/vc/src
rm -rf lib/modules || true
tar xzf ../../output/shared.tgz
tar czf ../../output/shared.tgz *

echo Build complete. Result is in \'output\' directory 
