AM_CPPFLAGS = \
	-I$(top_srcdir)/src/libstrongswan \
	-I$(top_srcdir)/src/libcharon

AM_CFLAGS = \
	$(PLUGIN_CFLAGS)

if MONOLITHIC
noinst_LTLIBRARIES = libstrongswan-kernel-wintun.la
else
plugin_LTLIBRARIES = libstrongswan-kernel-wintun.la
endif

libstrongswan_kernel_wintun_la_SOURCES = \
	kernel_wintun_plugin.h kernel_wintun_plugin.c \
	kernel_wintun_provider.h kernel_wintun_provider.c

libstrongswan_kernel_wintun_la_LDFLAGS = -module -avoid-version
