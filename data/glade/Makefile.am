appdatadir = $(pkgdatadir)/data/glade
appdata_DATA =				\
	gtkpod.glade

EXTRA_DIST = 				\
	$(appdata_DATA)

include ../directories.mk

all-local: create-glade-link

# Creating symbolic link to glade file in installed glade directory
create-glade-link: create-glade-dir
	if test -e $(gtkpod_glade_dir)/$(appdata_DATA); then \
      # File already exists. Replacing ..." \
      sudo rm -f $(gtkpod_glade_dir)/$(appdata_DATA); \
	fi; \
    # Creating link for $(appdata_DATA)" \
	sudo ln -s `pwd`/$(appdata_DATA) $(gtkpod_glade_dir)/$(appdata_DATA);

# Clean up the links and files created purely for dev testing
clean-local: clean-glade-dir

clean-glade-dir:
	if test -h $(gtkpod_glade_dir)/$(appdata_DATA); then \
      # Symbolic link exists. Removing ..." \
      sudo rm -f $(gtkpod_glade_dir)/$(appdata_DATA); \
	fi;