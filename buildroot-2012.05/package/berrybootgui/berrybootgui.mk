#############################################################
#
# Berryboot GUI
#
#############################################################


BERRYBOOTGUI_VERSION=1.0
BERRYBOOTGUI_SITE=$(TOPDIR)/../BerrybootInstaller
BERRYBOOTGUI_SITE_METHOD=local
BERRYBOOTGUI_INSTALL_STAGING = NO
BERRYBOOTGUI_DEPENDENCIES=qt

define BERRYBOOTGUI_BUILD_CMDS
	(cd $(@D) ; $(QT_QMAKE))
	$(MAKE) -C $(@D) all
	$(TARGET_STRIP) $(@D)/BerrybootInstaller
endef

define BERRYBOOTGUI_INSTALL_TARGET_CMDS
#	$(INSTALL) -m 0755 $(@D)/BerrybootInstaller $(TOPDIR)/output/images
	gzip -c $(@D)/BerrybootInstaller > $(TOPDIR)/output/images/BerrybootInstaller.gz
endef

$(eval $(call GENTARGETS))
