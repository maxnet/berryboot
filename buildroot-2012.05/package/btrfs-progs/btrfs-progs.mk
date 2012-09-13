#############################################################
#
# BTRFS-PROGS
#
#############################################################

BTRFS_PROGS_VERSION = master
BTRFS_PROGS_SITE = git://git.kernel.org/pub/scm/linux/kernel/git/arne/btrfs-progs.git
BTRFS_PROGS_INSTALL_STAGING = NO
BTRFS_PROGS_DEPENDENCIES = util-linux attr

define BTRFS_PROGS_BUILD_CMDS
 	$(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) all
endef

define BTRFS_PROGS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/btrfs $(TARGET_DIR)/sbin
    $(INSTALL) -D -m 0755 $(@D)/btrfsck $(TARGET_DIR)/sbin
    $(INSTALL) -D -m 0755 $(@D)/mkfs.btrfs $(TARGET_DIR)/sbin
endef

$(eval $(call GENTARGETS))

