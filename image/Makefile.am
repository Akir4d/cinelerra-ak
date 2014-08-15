desktopdir = $(datadir)/applications
desktop_DATA = cinelerra-cv.desktop
bin_SCRIPTS = cinelerra-cv
CLEANFILES = $(bin_SCRIPTS) $(desktop_DATA)

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = cinelerra-cv.xpm

EXTRA_DIST = \
        cinelerra-cv.desktop.in cinelerra-cv.xpm cinelerra-cv.in

do_subst = sed -e 's,[@]bindir[@],$(bindir),g'

cinelerra-cv: cinelerra-cv.in Makefile
			$(do_subst) < $(srcdir)/cinelerra-cv.in > cinelerra-cv
			chmod +x cinelerra-cv

cinelerra-cv.desktop: cinelerra-cv.desktop.in Makefile
			$(do_subst) < $(srcdir)/cinelerra-cv.desktop.in > cinelerra-cv.desktop
			chmod +x cinelerra-cv.desktop
      
