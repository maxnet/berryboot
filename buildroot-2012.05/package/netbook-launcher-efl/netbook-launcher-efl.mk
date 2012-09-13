#############################################################
#
# NETBOOK_LAUNCHER_EFL
#
#############################################################

NETBOOK_LAUNCHER_EFL_VERSION = 0.3.2
NETBOOK_LAUNCHER_EFL_SOURCE = netbook-launcher-efl_$(NETBOOK_LAUNCHER_EFL_VERSION).orig.tar.gz
NETBOOK_LAUNCHER_EFL_SITE = http://archive.ubuntu.com/ubuntu/pool/universe/n/netbook-launcher-efl
NETBOOK_LAUNCHER_EFL_INSTALL_STAGING = YES
NETBOOK_LAUNCHER_EFL_INSTALL_TARGET = YES
NETBOOK_LAUNCHER_EFL_CONF_OPT = 
NETBOOK_LAUNCHER_EFL_DEPENDENCIES = libecore libedje libeet libeina libelementary libevas libefreet
NETBOOK_LAUNCHER_EFL_AUTORECONF = YES

$(eval $(call AUTOTARGETS))
