#!/bin/sh

if [ -e output/target/lib/modules ]; then

	echo Making shared.tgz with modules
	(cd output/target ; cp ../staging/sbin/iscsistart ./sbin ; tar czvf ../images/shared.tgz lib/modules lib/firmware sbin/iscsistart)
	rm -rf output/target/lib/modules
	rm output/target/sbin/iscsistart
	rm -rf output/target/lib/firmware || true
fi


# Remove redudant fonts
if [ -e output/target/usr/lib/fonts/VeraBd.ttf ]; then
	rm output/target/usr/lib/fonts/*
	cp output/staging/usr/lib/fonts/DejaVuSans.ttf output/target/usr/lib/fonts
	cp output/staging/usr/lib/fonts/DejaVuSans-Bold.ttf output/target/usr/lib/fonts
fi

