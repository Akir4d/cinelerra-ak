if ALT_SCRIPT_PREFIX
BNDIR = /usr/bin
DTDIR = /usr/share
else
BNDIR = $(bindir)
DTDIR = $(prefix)/share
endif
desktopdir = ${DTDIR}/applications
desktop_DATA = cinelerra-cva.desktop
bin_SCRIPTS = cinelerra-cva
CLEANFILES = $(bin_SCRIPTS) $(desktop_DATA)

pixmapdir = ${DTDIR}/pixmaps
pixmap_DATA = cinelerra-cva.xpm

EXTRA_DIST = \
        cinelerra-cva.desktop.in cinelerra-cva.xpm cinelerra-cva.in

do_subst = sed -e 's,[@]bindir[@],$(bindir),g'
do_subst_desk = sed -e 's,[@]BNDIR[@],$(BNDIR),g'

cinelerra-cva: cinelerra-cva.in Makefile
			$(do_subst) < $(srcdir)/cinelerra-cva.in > cinelerra-cva
			chmod +x cinelerra-cva

cinelerra-cva.desktop: cinelerra-cva.desktop.in Makefile
			$(do_subst_desk) < $(srcdir)/cinelerra-cva.desktop.in > cinelerra-cva.desktop
			chmod +x cinelerra-cva.desktop

install: cinelerra-cva cinelerra-cva.desktop cinelerra-cva.xpm
	mv -f cinelerra-cva ${BNDIR}
	mv -f cinelerra-cva.desktop ${DTDIR}/applications
	cp -f cinelerra-cva.xpm ${DTDIR}/pixmaps
	cp -f cinelerra-cva.conf /etc/sysctl.d/50-cinelerra-cva.conf
	-echo "0x7fffffff" > /proc/sys/kernel/shmmax
	
uninstall: cinelerra-cva cinelerra-cva.desktop cinelerra-cva.xpm
	rm -f ${BNDIR}/cinelerra-cva
	rm -f  ${DTDIR}/applications/cinelerra-cva.desktop
	rm -f  ${DTDIR}/pixmaps/cinelerra-cva.xpm
	rm -f /etc/sysctl.d/50-cinelerra-cva.conf

      
