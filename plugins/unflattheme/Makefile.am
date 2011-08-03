SUBDIRS = data

plugin_LTLIBRARIES = unflattheme.la
unflattheme_la_LDFLAGS = -avoid-version -module -shared 
unflattheme_la_LIBADD = data/libimagedata.a
unflattheme_la_SOURCES = unflattheme.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = unflattheme.h 
