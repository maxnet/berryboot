#!/bin/sh

if [ -e output/target/lib/modules ]; then

	echo Making shared.img with modules
	mkdir -p output/shared/lib
	cp -af output/target/lib/modules output/shared/lib
	cp -af output/target/lib/firmware output/shared/lib || true
	rm -rf output/target/lib/modules output/target/lib/firmware || true
	#(cd output/shared ; tar czvf ../images/shared.tgz *)
	mksquashfs output/shared output/images/shared.img -all-root -noappend
fi

# Remove redudant fonts
if [ -e output/target/usr/lib/fonts/VeraBd.ttf ]; then
	rm output/target/usr/lib/fonts/*
	cp output/staging/usr/lib/fonts/DejaVuSans.ttf output/target/usr/lib/fonts
	cp output/staging/usr/lib/fonts/DejaVuSans-Bold.ttf output/target/usr/lib/fonts
fi

if [ -e output/target/etc/udev/hwdb.d ]; then
	rm -rf output/target/etc/udev/hwdb.d
fi

# Remove libvncclient, only using server libs
if [ -e output/target/usr/lib/libvncclient.so ]; then
	rm output/target/usr/lib/libvncclient*
fi

# Only using mkfs.btrfs, remove other large utilities
rm -f output/target/usr/bin/btrfs*

# Only using libdevmapper, remove LVM utilities
rm -f output/target/usr/sbin/dmsetup output//targetusr/sbin/dmeventd output/target/usr/sbin/integritysetup \
	output/target/usr/sbin/veritysetup output//targetusr/sbin/blkdeactivate


# Not supporting reencrypt
rm -f output/target/usr/sbin/cryptsetup-reencrypt

# Remove redundant sysutil-linux utilities
rm -f output//targetusr/sbin/rtcwake output//targetusr/sbin/ldattach output/target/usr/sbin/readprofile \
	output/target/usr/bin/uuidgen output/target/usr/bin/uuidparse output/target/usr/bin/whereis \
	output/target/usr/bin/setsid \
	output/target/sbin/swaplabel output/target/sbin/swapon output/target/sbin/swapoff

if [ -e output/target/usr/lib/qt/plugins ]; then
	rm -rf output/target/usr/lib/qt
fi

rm -rf output/target/etc/ssl/man

