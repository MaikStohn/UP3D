include $(TOPDIR)/rules.mk

PKG_NAME:=up3dwifi
PKG_VERSION:=2016-04-15
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/MaikStohn/UP3D.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=master
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_MAINTAINER:=Maik Stohn <git@stohn.de>

include $(INCLUDE_DIR)/package.mk

define Package/up3dwifi
	SECTION:=UP3D
	CATEGORY:=UP3D
	DEPENDS:=+libusb-1.0
	TITLE:=UP3D WiFi USB bridge daemon
endef

define Build/Prepare
	$(Build/Prepare/Default)
endef

define Build/Compile
	$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include -I$(PKG_BUILD_DIR)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		$(MAKE) -C $(PKG_BUILD_DIR)/UP3DWIFI/up3dwifisrv all
endef

define Package/up3dwifi/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/UP3DWIFI/up3dwifisrv/up3dwifisrv $(1)/usr/bin/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/UP3DWIFI/up3dwifisrv/up3dwifilogo $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/hotplug.d/usb
	$(INSTALL_DATA) ./up3dwifi.hotplug $(1)/etc/hotplug.d/usb/11-up3dwifi
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./up3dwifi.initd $(1)/etc/init.d/up3dwifilogo
#	$(INSTALL_DIR) $(1)/etc/modules.d
#	$(INSTALL_DATA) ./up3dwifi.modulesd $(1)/etc/modules.d/i2c-gpio-custom
endef

$(eval $(call BuildPackage,up3dwifi))

