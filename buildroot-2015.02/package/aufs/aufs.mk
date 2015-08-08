################################################################################
#
# AUFS download helper
#
################################################################################

AUFS_VERSION = $(call qstrip,$(BR2_LINUX_KERNEL_EXT_AUFS_VERSION))
AUFS_SITE = git://github.com/sfjro/aufs4-standalone.git
AUFS_LICENSE = GPLv2

ifeq ($(BR2_LINUX_KERNEL_EXT_AUFS),y)
ifeq ($(AUFS_VERSION),)
$(error BR2_LINUX_KERNEL_EXT_AUFS_VERSION must be set to the AUFS branch matching your kernel, e.g. "aufs3.18.1+")
endif
endif

$(eval $(generic-package))
