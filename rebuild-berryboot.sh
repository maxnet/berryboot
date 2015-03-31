#!/bin/sh

set -e

# Force rebuild of BerrybootGUI
if [ -e buildroot-2015.02/output/build/berrybootgui2-1.0 ]; then
	(cd buildroot-2015.02 ; make berrybootgui2-dirclean)
fi

./build-berryboot.sh "$@"
