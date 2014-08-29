SUBDIRS = data

plugin_LTLIBRARIES = defaulttheme.la
defaulttheme_la_LDFLAGS = -avoid-version -module -shared 
defaulttheme_la_LIBADD = data/libimagedata.a
defaulttheme_la_SOURCES = defaulttheme.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = defaulttheme.h 
