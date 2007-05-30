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
	    rm -f $(DESTDIR)$(pkgdatadir)/pixmaps/$$gladefile; \
	    ln -s ../$$gladefile $(DESTDIR)$(pkgdatadir)/pixmaps/$$gladefile; \
	  fi \
	done

uninstall-hook:
	if test -d $(DESTDIR)$(pkgdatadir)/pixmaps; then \
	      rm -fr $(DESTDIR)$(pkgdatadir)/pixmaps/ ; \
	fi
	for gladefile in gtkpod.glade gtkpod.gladep; do \
	    rm -f $(DESTDIR)$(pkgdatadir)/$$gladefile; \
	done
	if test -d $(DESTDIR)$(pkgdatadir)/scripts ; then \
	    rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir)/scripts; \
	fi
	if test -d $(DESTDIR)$(pkgdatadir) ; then \
	    rmdir --ignore-fail-on-non-empty $(DESTDIR)$(pkgdatadir); \
	fi

dist-hook:
	for gladefile in gtkpod.glade gtkpod.gladep; do \
	    rm -f $(distdir)/pixmaps/$$gladefile; \
	done
	rm -rf $(distdir)/pixmaps/.svn


