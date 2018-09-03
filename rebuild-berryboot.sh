#!/bin/sh

set -e

BUILDROOT=buildroot-2018.08

# Force rebuild of BerrybootGUI
if [ -e $BUILDROOT/output/build/berrybootgui2-1.0 ]; then
	make -C $BUILDROOT berrybootgui2-dirclean
fi

./build-berryboot.sh "$@"
