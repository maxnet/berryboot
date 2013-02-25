#############################################################
#
# Berryboot GUI
#
#############################################################


BERRYBOOTGUI2_VERSION=1.0
BERRYBOOTGUI2_SITE=$(TOPDIR)/../BerrybootGUI2.0
BERRYBOOTGUI2_SITE_METHOD=local
BERRYBOOTGUI2_INSTALL_STAGING = NO
BERRYBOOTGUI2_DEPENDENCIES=qt rpi-userland openssl libcurl

define BERRYBOOTGUI2_BUILD_CMDS
	(cd $(@D) ; $(QT_QMAKE))
	$(MAKE) -C $(@D) all
	$(TARGET_STRIP) $(@D)/BerrybootInstaller
endef

define BERRYBOOTGUI2_INSTALL_TARGET_CMDS
#	gzip -c $(@D)/BerrybootInstaller > $(TOPDIR)/output/images/BerrybootInstaller.gz
        $(INSTALL) -m 0755 $(@D)/BerrybootInstaller $(TARGET_DIR)/usr/bin/BerrybootGUI
        $(INSTALL) -D -m 0644 package/berrybootgui2/interfaces $(TARGET_DIR)/etc/network/interfaces
        rm $(TARGET_DIR)/init || true
        $(INSTALL) -m 0755 package/berrybootgui2/init $(TARGET_DIR)/init
        $(INSTALL) -m 0755 package/berrybootgui2/chroot_image $(TARGET_DIR)/usr/sbin
        $(INSTALL) -m 0755 package/berrybootgui2/hotplug $(TARGET_DIR)/sbin/hotplug
        $(INSTALL) -m 0755 $(STAGING_DIR)/opt/vc/lib/libvchiq_arm.so $(TARGET_DIR)/usr/lib
        $(INSTALL) -m 0755 $(STAGING_DIR)/opt/vc/lib/libvcos.so $(TARGET_DIR)/usr/lib
        $(INSTALL) -m 0755 $(STAGING_DIR)/opt/vc/lib/libbcm_host.so $(TARGET_DIR)/usr/lib
endef

$(eval $(call GENTARGETS))
