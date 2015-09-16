#!/bin/sh

if [ -e output/target/lib/modules ]; then

	echo Making shared.tgz with modules
	mkdir -p output/shared/lib
	cp -af output/target/lib/modules output/shared/lib
	cp -af output/target/lib/firmware output/shared/lib || true
	rm -rf output/target/lib/modules output/target/lib/firmware || true
	(cd output/shared ; tar czvf ../images/shared.tgz *)
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

