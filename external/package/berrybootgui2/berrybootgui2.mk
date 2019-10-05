#############################################################
#
# Berryboot GUI
#
#############################################################


BERRYBOOTGUI2_VERSION=1.0
BERRYBOOTGUI2_SITE=$(TOPDIR)/../BerrybootGUI2.0
BERRYBOOTGUI2_SITE_METHOD=local
BERRYBOOTGUI2_INSTALL_STAGING = NO
BERRYBOOTGUI2_DEPENDENCIES=qt rpi-userland openssl libcurl wpa_supplicant util-linux

define BERRYBOOTGUI2_BUILD_CMDS
	(cd $(@D) ; $(QT_QMAKE))
	$(MAKE) -C $(@D) all
	$(TARGET_STRIP) $(@D)/BerrybootInstaller
endef

define BERRYBOOTGUI2_INSTALL_TARGET_CMDS
        $(INSTALL) -m 0755 $(@D)/BerrybootInstaller $(TARGET_DIR)/usr/bin/BerrybootGUI
        rm -f $(TARGET_DIR)/init
        $(INSTALL) -m 0755 $(BERRYBOOTGUI2_PKGDIR)/init $(TARGET_DIR)/init
        $(INSTALL) -m 0755 $(BERRYBOOTGUI2_PKGDIR)/chroot_image $(TARGET_DIR)/usr/sbin
        $(INSTALL) -m 0755 $(BERRYBOOTGUI2_PKGDIR)/hotplug $(TARGET_DIR)/sbin/hotplug
        $(INSTALL) -D -m 0644 $(BERRYBOOTGUI2_PKGDIR)/blacklist-dvb.conf $(TARGET_DIR)/etc/modprobe.d/blacklist-dvb.conf
	$(INSTALL) -D -m 0644 $(BERRYBOOTGUI2_PKGDIR)/blacklist-drm.conf $(TARGET_DIR)/etc/modprobe.d/blacklist-drm.conf
	$(INSTALL) -D -m 0644 $(BERRYBOOTGUI2_PKGDIR)/install-i2cdev.conf $(TARGET_DIR)/etc/modprobe.d/install-i2cdev.conf
endef

$(eval $(generic-package))
