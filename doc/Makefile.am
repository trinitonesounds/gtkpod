## File created by the gnome-build tools

SUBDIRS = figures

docdir = $(pkgdatadir)/doc
doc_DATA = \
	gtkpod.xml \
	adding-cover-art-from-a-browser.html \
	adding-files-and-folders.html \
	advanced-features.html \
	copying-content-to-the-ipod.html \
	cover-art.html \
	exporting-files-from-the-ipod.html \
	getting-started.html \
	gtkpod.html \
	gtkpod-info.html \
	hooking-up-the-ipod.html \
	ipod-and-content.html \
	ipod-file-recovery.html \
	layout-of-gtkpod.html \
	managing-playlists.html \
	playcounts-and-ratings.html \
	playlists-podcasts-and-photos.html \
	setting-ipod-properties.html \
	the-sysinfo-file.html \
	troubleshooting-faq.html \
	view-and-edit-track-details.html

EXTRA_DIST = $(doc_DATA)

update-docs:
		xsltproc /usr/share/xml/gnome/xslt/docbook/html/db2html.xsl gtkpod.xml > gtkpod.html
