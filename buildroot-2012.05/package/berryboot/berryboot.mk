#############################################################
#
# Berryboot
#
#############################################################


BERRYBOOT_VERSION=1.0
BERRYBOOT_SITE=$(TOPDIR)/../BerrybootUI
BERRYBOOT_SITE_METHOD=local
BERRYBOOT_INSTALL_STAGING = NO
BERRYBOOT_DEPENDENCIES=dialog ncurses

define BERRYBOOT_BUILD_CMDS
	$(MAKE) CXX="$(TARGET_CXX)" LD="$(TARGET_LD)" CXXFLAGS="$(CXXFLAGS) -I$(DIALOG_DIR)" LDFLAGS="$(LDFLAGS) -L$(DIALOG_DIR) -lncurses -ldialog" -C $(@D) all
endef

define BERRYBOOT_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/BerrybootUI $(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0644 package/berryboot/interfaces $(TARGET_DIR)/etc/network/interfaces
	rm $(TARGET_DIR)/init || true
	$(INSTALL) -m 0755 package/berryboot/init $(TARGET_DIR)/init
endef

$(eval $(call GENTARGETS))
