#############################################################
#
# Cryptsetup 
#
#############################################################

CRYPTSETUP_VERSION=1.5.1
CRYPTSETUP_SITE=https://cryptsetup.googlecode.com/files
CRYPTSETUP_SOURCE=cryptsetup-$(CRYPTSETUP_VERSION).tar.bz2
CRYPTSETUP_INSTALL_STAGING = YES
CRYPTSETUP_INSTALL_TARGET = NO 
CRYPTSETUP_DEPENDENCIES=lvm2 openssl popt util-linux
CRYPTSETUP_CONF_OPT=--with-crypto_backend=kernel --enable-static-cryptsetup

$(eval $(call AUTOTARGETS))
