berryboot
=========

Berryboot -- Boot menu / OS installer for ARM devices

Author: Floris Bos < bos AT je-eigen-domein DOT nl > 
License: Simplified BSD - see LICENSE.berryboot for details
Programming language: C++


===
Folders
===

BerrybootUI - source of textual boot menu (uses libdialog)
BerrybootInstaller - source of graphical user interface for installation and editing the boot menu (uses Qt)
buildroot-2012.05 - build system to create a minimal Linux operating system to run the boot menu under


===
To build
===

Simply run ./rebuild-berryboot.sh
The files in the output folder must be copied to an empty FAT formatted SD card, together with the Raspberry Pi firmware files from https://github.com/raspberrypi/firmware/tree/master/boot

 
