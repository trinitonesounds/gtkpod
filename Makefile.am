## Process this file with automake to produce Makefile.in

SUBDIRS = src po scripts

EXTRA_DIST = \
	autogen.sh \
	gtkpod.glade \
	gtkpod.gladep \
	TROUBLESHOOTING \
	TODOandBUGS.txt

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir); \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir); \
	    fi \
	  done \
	fi

	if test -f gtkpod.glade; then \
	  $(INSTALL_DATA) gtkpod.glade $(DESTDIR)$(pkgdatadir); \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

