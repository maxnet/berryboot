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
#cp output/images/shared.tgz output/images/BerrybootInstaller.gz ../output
cp output/images/shared.tgz ../output
cp output/build/linux-HEAD/arch/arm/boot/zImage ../output/kernel_berryboot.img
#cp output/images/zImage ../output/kernel_rpi_aufs.img
#cp output/images/rootfs.cpio.gz ../output/berryboot.img
echo Build complete. Result is in \'output\' directory 
