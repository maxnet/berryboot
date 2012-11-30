#############################################################
#
# Raspberry Pi userland libraries
#
#############################################################

RPI_USERLAND_VERSION = next
RPI_USERLAND_SOURCE = rpi-userland-$(RPI_USERLAND_VERSION).tar.gz
RPI_USERLAND_SITE = git://github.com/raspberrypi/userland.git
RPI_USERLAND_INSTALL_STAGING = YES
RPI_USERLAND_INSTALL_TARGET = NO

$(eval $(call CMAKETARGETS))
