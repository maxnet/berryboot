################################################################################
#
# rpi-wifi-firmware
#
################################################################################

RPI_WIFI_FIRMWARE_VERSION = 5113681d6dcd46581a1882cbeb3d5cf1ddbf7676
RPI_WIFI_FIRMWARE_SITE = $(call github,RPi-Distro,firmware-nonfree,$(RPI_WIFI_FIRMWARE_VERSION))
RPI_WIFI_FIRMWARE_LICENSE = PROPRIETARY
RPI_WIFI_FIRMWARE_LICENSE_FILES = LICENCE.broadcom_bcm43xx

define RPI_WIFI_FIRMWARE_INSTALL_TARGET_CMDS
	$(INSTALL) -d $(TARGET_DIR)/lib/firmware/brcm
	$(INSTALL) -m 0644 $(@D)/brcm/brcmfmac43430* $(TARGET_DIR)/lib/firmware/brcm
	$(INSTALL) -m 0644 $(@D)/brcm/brcmfmac43455* $(TARGET_DIR)/lib/firmware/brcm
	$(INSTALL) -m 0644 $(@D)/brcm/brcmfmac43456* $(TARGET_DIR)/lib/firmware/brcm
endef

$(eval $(generic-package))
