## Process this file with automake to produce Makefile.in

SUBDIRS = src po scripts


EXTRA_DIST = \
	autogen.sh \
	gtkpod.glade \
	gtkpod.gladep \
	TROUBLESHOOTING \
	TODOandBUGS.txt \
	pixmaps

install-data-hook:
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      if ! test -h $$pixmap; then \
	        $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	      fi \
	    fi \
	  done \
	fi
	for gladefile in gtkpod.glade gtkpod.gladep; do \
	  if test -f $$gladefile; then \
          $(INSTALL_DATA) $$gladefile $(DESTDIR)$(pkgdatadir); \
	  ln -s $(DESTDIR)$(pkgdatadir)/$$gladefile $(DESTDIR)$(pkgdatadir)/pixmaps/$$gladefile; \
	  fi \
	done

uninstall-hook:
	if test -d $(DESTDIR)$(pkgdatadir)/pixmaps; then \
	      rm -fr $(DESTDIR)$(pkgdatadir)/pixmaps/ ; \
	fi
	for gladefile in gtkpod.glade gtkpod.gladep; do \
	    rm -f $(DESTDIR)$(pkgdatadir)/$$gladefile; \
	done

dist-hook:
	for gladefile in gtkpod.glade gtkpod.gladep; do \
	    rm -f $(distdir)/pixmaps/$$gladefile; \
	done
	rm -rf $(distdir)/pixmaps/CVS


