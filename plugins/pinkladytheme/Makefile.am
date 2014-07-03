SUBDIRS = data

plugin_LTLIBRARIES = pinkladytheme.la
pinkladytheme_la_LDFLAGS = -avoid-version -module -shared 
pinkladytheme_la_LIBADD = data/libimagedata.a
pinkladytheme_la_SOURCES = pinkladytheme.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = pinkladytheme.h 
