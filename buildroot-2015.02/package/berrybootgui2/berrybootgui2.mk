#############################################################
#
# Berryboot GUI
#
#############################################################


BERRYBOOTGUI2_VERSION=1.0
BERRYBOOTGUI2_SITE=$(TOPDIR)/../BerrybootGUI2.0
BERRYBOOTGUI2_SITE_METHOD=local
BERRYBOOTGUI2_INSTALL_STAGING = NO
BERRYBOOTGUI2_DEPENDENCIES=qt rpi-userland openssl libcurl wpa_supplicant

define BERRYBOOTGUI2_BUILD_CMDS
	(cd $(@D) ; $(QT_QMAKE))
	$(MAKE) -C $(@D) all
	$(TARGET_STRIP) $(@D)/BerrybootInstaller
endef

define BERRYBOOTGUI2_INSTALL_TARGET_CMDS
        $(INSTALL) -m 0755 $(@D)/BerrybootInstaller $(TARGET_DIR)/usr/bin/BerrybootGUI
        rm $(TARGET_DIR)/init || true
        $(INSTALL) -m 0755 package/berrybootgui2/init $(TARGET_DIR)/init
        $(INSTALL) -m 0755 package/berrybootgui2/chroot_image $(TARGET_DIR)/usr/sbin
	$(INSTALL) -m 0755 package/berrybootgui2/hotplug $(TARGET_DIR)/sbin/hotplug
endef

$(eval $(generic-package))
