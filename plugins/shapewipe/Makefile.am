plugin_LTLIBRARIES = shapewipe.la
shapewipe_la_LDFLAGS = -avoid-version -module -shared 
shapewipe_la_LIBADD = 
shapewipe_la_SOURCES = shapewipe.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS) \
	-DDEFAULT_SHAPE=\"$(shapewipedir)/heart.png\"

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

shapewipedir = $(plugindir)/shapewipe
shapewipe_DATA = \
	circle.png \
	clock.png \
	heart.png \
	tile2x2h.png \
	tile2x2v.png \
	BAR-fiona.png \
	BAR-kubuntu.png \
	BAR-spiral.png \
	BAR-trans-1.png \
	BAR-trans-2.png \
	BAR-trans-3.png \
	BAR-trans-4.png \
	BAR-trans-5.png \
	BAR-woma1.png \
	BAR-woma2.png \
	Fleur-de-lis-of-Florence.png \
	HJ-helix2.png \
	HJ-helix4.png \
	HJ-xtype.png \
	JST-cicle-vertical-invert.png \
	JST-circle-vertical.png \
	JST-demi-cercle.png \
	JST-demi-circle-horizontal-haut.png \
	JST-demi-circle-non-opposite.png \
	JST-demi-circle-opposite-invert.png \
	JST-demi-circle-opposite.png \
	JST-demi-circle-together.png \
	JST-demi-circle-verticale.png \
	JST-join.png JST-multiple-circle.png \
	JST-profondeur.png \
	JST-quadri-circle.png \
	JST-quadrillage1.png \
	JST-quadrillage.png \
	JST-spiral.png \
	JST-tile-circle.png \
	JST-triangle.png \
	spirally1.png \
	swirl1.png

noinst_HEADERS = shapewipe.h
EXTRA_DIST = picon.png $(shapewipe_DATA)

BUILT_SOURCES = picon_png.h
picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }
clean-local:
	$(RM) picon_png.h
