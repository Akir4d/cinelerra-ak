SUBDIRS = data

plugin_LTLIBRARIES = hulktheme.la
hulktheme_la_LDFLAGS = -avoid-version -module -shared 
hulktheme_la_LIBADD = data/libimagedata.a
hulktheme_la_SOURCES = hulktheme.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = hulktheme.h 
