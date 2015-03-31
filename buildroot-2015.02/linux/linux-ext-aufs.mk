################################################################################
# Linux aufs extension
#
# Patch the linux kernel with aufs extension
################################################################################

ifeq ($(BR2_LINUX_KERNEL_EXT_AUFS),y)
# Add dependency to aufs package (download helper for the aufs source)
LINUX_DEPENDENCIES += aufs

AUFS_PATCHES = kbuild base mmap standalone

define AUFS_PREPARE_KERNEL
	$(foreach p,$(AUFS_PATCHES),./support/scripts/apply-patches.sh $(LINUX_DIR) $(AUFS_DIR) *$(p).patch;)
	cp -af $(AUFS_DIR)/fs $(AUFS_DIR)/Documentation $(LINUX_DIR)
	cp -af $(AUFS_DIR)/include/uapi/linux/aufs_type.h \
		$(LINUX_DIR)/include/uapi/linux
	cp -af $(AUFS_DIR)/include/uapi/linux/aufs_type.h \
		$(STAGING_DIR)/usr/include/linux
endef

LINUX_PRE_PATCH_HOOKS += AUFS_PREPARE_KERNEL

endif #BR2_LINUX_KERNEL_EXT_AUFS
