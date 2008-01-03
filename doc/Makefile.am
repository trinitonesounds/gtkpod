## File created by the gnome-build tools

SUBDIRS = figures

docdir = $(pkgdatadir)/doc
doc_DATA = \
	figures \
	gtkpod.xml \
	*.html

EXTRA_DIST = $(doc_DATA)

update-docs:
		xsltproc /usr/share/xml/gnome/xslt/docbook/html/db2html.xsl gtkpod.xml > gtkpod.html
