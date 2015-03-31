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

