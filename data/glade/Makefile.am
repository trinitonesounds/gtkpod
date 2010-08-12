appdatadir = $(pkgdatadir)/data/glade
appdata_DATA =				\
	gtkpod.glade

all-local:
	$(INTLTOOL_EXTRACT) -type=gettext/glade gtkpod.glade

EXTRA_DIST = 				\
	$(appdata_DATA)
