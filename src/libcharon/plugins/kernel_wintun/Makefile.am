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
	kernel_wintun_api.h kernel_wintun_api.c \
	kernel_wintun_device.h kernel_wintun_device.c \
	kernel_wintun_provider.h kernel_wintun_provider.c

libstrongswan_kernel_wintun_la_LDFLAGS = -module -avoid-version

TESTS = kernel_wintun_tests

check_PROGRAMS = $(TESTS)

kernel_wintun_tests_SOURCES = \
	tests.h tests.c \
	suites/test_provider.c \
	kernel_wintun_api.c \
	kernel_wintun_device.c \
	kernel_wintun_provider.c

kernel_wintun_tests_CFLAGS = \
	-I$(top_srcdir)/src/libstrongswan \
	-I$(top_srcdir)/src/libstrongswan/tests \
	@COVERAGE_CFLAGS@

kernel_wintun_tests_LDFLAGS = @COVERAGE_LDFLAGS@
kernel_wintun_tests_LDADD = \
	$(top_builddir)/src/libstrongswan/libstrongswan.la \
	$(top_builddir)/src/libstrongswan/tests/libtest.la
