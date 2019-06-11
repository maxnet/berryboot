Berryboot
=========

```
Berryboot -- Boot menu / OS installer for ARM devices

Author: Floris Bos <bos AT je-eigen-domein DOT nl> 
License: Simplified BSD - see LICENSE.berryboot for details
Programming language: C++

Source code available at: https://github.com/maxnet/berryboot
```

Website: http://www.berryterminal.com/doku.php/berryboot 


Folders
===

BerrybootGUI2.0 - source of the graphical boot menu and installer interface (uses Qt)
buildroot - build system to create a minimal Linux operating system to run the boot menu under
buildroot/package/berrybootgui2/init - script that gets executed on boot, starts BerrybootGUI


Build requirements
===

Berryboot uses Buildroot to build a minimal Linux operating system to run under.
Buildroot requires that the following packages are installed: http://www.buildroot.org/downloads/manual/manual.html#requirement


To build for the Raspberry Pi
===

```
./build-berryboot.sh device_pi0123
```

The files in the output folder must be copied to an empty FAT formatted SD card. 


To add support for a new ARM device
===

Create a file buildroot/berryboot-configs/device-mydevice with the buildroot configuration options to build a kernel and supporting files for your device. 
E.g.:

```
BR2_LINUX_KERNEL=y
BR2_LINUX_KERNEL_CUSTOM_GIT=y
BR2_LINUX_KERNEL_CUSTOM_REPO_URL="https://github.com/mydevice/linux.git"
BR2_LINUX_KERNEL_CUSTOM_REPO_VERSION="branch3.18"
BR2_LINUX_KERNEL_DEFCONFIG="mydevice"
# Kernel options needed by Berryboot (enables AUFS support and such)
BR2_LINUX_KERNEL_CONFIG_FRAGMENT_FILES="../configs/kernel_config_fragment_berryboot"
# Install AUFS kernel patch
BR2_LINUX_KERNEL_EXT_AUFS=y
BR2_LINUX_KERNEL_EXT_AUFS_VERSION="aufs3.18"
BR2_LINUX_KERNEL_ZIMAGE=y
BR2_LINUX_KERNEL_IMAGE_INSTALL_NAME="kernel_mydevice_aufs.img"
```

AUFS kernel extension version must match kernel version.
Build with:

```
./build-berryboot.sh device_mydevice
```

Berryboot expects that the kernel cmdline parameters are stored in a text file called cmdline.txt on the SD card or are stored at the end of the file uEnv.txt, and that the parameters can be edited (so may not be stored in a binary format with checksum).
You must configure your bootloader to read the cmdline parameters, and boot Linux kernel kernel_mydevice_aufs.img with initramfs berryboot.img.
