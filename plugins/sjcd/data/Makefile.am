profilesdir = $(pkgdatadir)/data
profiles_DATA = rhythmbox.gep
dist_profiles_DATA = rhythmbox.gep

# gsettings_SCHEMAS is a list of all the schemas you want to install
schema_in_files = org.gtkpod.sjcd.gschema.xml.in
gsettings_SCHEMAS = $(schema_in_files:.gschema.xml.in=.gschema.xml)

# include the appropriate makefile rules for schema handling
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(profiles_DATA) \
	$(schema_in_files)

CLEANFILES = $(gsettings_SCHEMAS)

all-local: create-data-links

# Creating symbolic links in plugin root directory
create-data-links:
	if [ ! -e $(top_srcdir)/data/$(profiles_DATA) ]; then \
		$(LN_S) `pwd`/$(profiles_DATA) $(top_srcdir)/data/$(profiles_DATA); \
	fi;

