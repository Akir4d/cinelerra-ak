DTDIR = /usr/share
BNDIR = /usr/bin
desktopdir = ${DTDIR}/applications
desktop_DATA = cinelerra-cv.desktop
bin_SCRIPTS = cinelerra-cv
CLEANFILES = $(bin_SCRIPTS) $(desktop_DATA)

pixmapdir = ${DTDIR}/pixmaps
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

install: cinelerra-cv cinelerra-cv.desktop cinelerra-cv.xpm
	mv -f cinelerra-cv ${BNDIR}
	mv -f cinelerra-cv.desktop ${DTDIR}/applications
	cp -f cinelerra-cv.xpm ${DTDIR}/pixmaps
	
uninstall: cinelerra-cv cinelerra-cv.desktop cinelerra-cv.xpm
	rm -f ${BNDIR}/cinelerra-cv
	rm -f  ${DTDIR}/applications/cinelerra-cv.desktop
	rm -f  ${DTDIR}/pixmaps/cinelerra-cv.xpm

      
