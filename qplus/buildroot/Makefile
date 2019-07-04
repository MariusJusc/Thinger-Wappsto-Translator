#
# Copyright (C) 2017 Seluxit ApS
#
# This is free software, licensed under the GNU General Public License v2.
#
#
#
include $(TOPDIR)/rules.mk

PKG_NAME:=bqplus
PKG_VERSION:=0.1.3
PKG_RELEASE:=0

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/Seluxit/qplus
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=1ee05955d4f2439d3cc8b2f7836ff85e7bd59b36
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_MAINTAINER:=Sigitas Dagilis<sigis@seluxit.com>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/bqplus
        SECTION:=utils
        CATEGORY:=Utilities
        TITLE:=QPlus - gateway - backend communication.
        DEPENDS:=+libev +libpthread +libopenssl +libstdcpp 
endef

define Package/bqplus/description
        Qlora is gateway software which communicates client app and Seluxit backend.
endef

define Package/bqplus/install
        $(INSTALL_DIR) $(1)/opt/qplus
        $(INSTALL_DIR) $(1)/opt/qplus/certs
        $(INSTALL_DIR) $(1)/opt/qplus/networks
        $(INSTALL_DIR) $(1)/opt/qplus/logs
        $(CP) $(PKG_INSTALL_DIR)/usr/bin/qplus.conf $(1)/opt/qplus/
        $(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/qplus $(1)/opt/qplus/
endef

$(eval $(call BuildPackage,bqplus))

