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

===
Folders
===

```
BerrybootGUI2.0 - source of the graphical boot menu and installer interface (uses Qt)
buildroot-2012.05 - build system to create a minimal Linux operating system to run the boot menu under
buildroot-2012.05/package/berrybootgui2/init - script that gets executed on boot, starts BerrybootGUI  
```

===
To build for Raspberry Pi
===

```
Simply run ./rebuild-berryboot.sh
The files in the output folder must be copied to an empty FAT formatted SD card, 
together with the Raspberry Pi firmware files from https://github.com/raspberrypi/firmware/tree/master/boot
```

===
To build for Allwinner A10 devices
===

```
Run ./rebuild-berryboot-a10.sh
The files in the output folder must be copied to an empty FAT formatted SD card,
together the script.bin file with the hardware information of your specific device.

In addition you need to build the u-boot bootloader manually, and dd it to the SD card.
See for more information: https://github.com/hno/uboot-allwinner/wiki

Alternatively you can also use the Android SD card writer tool, which takes care of
script.bin and u-boot SPL memory settings automatically.
Available in binary form at: http://get.berryboot.com/
Source available at: https://github.com/maxnet/android-sdcard-writer/
```

  
