appdatadir = $(pkgdatadir)/data
appdata_DATA =				\
	gtkpod.ui

EXTRA_DIST = 				\
	$(appdata_DATA)

	
all-local: create-ui-link

# Creating symbolic link to ui file in installed ui directory
create-ui-link: create-ui-dir
	if test -e $(gtkpod_ui_dir)/$(appdata_DATA); then \
      # File already exists. Replacing ..." \
      sudo rm -f $(gtkpod_ui_dir)/$(appdata_DATA); \
	fi; \
    # Creating link for $(appdata_DATA)" \
	sudo ln -s `pwd`/$(appdata_DATA) $(gtkpod_ui_dir)/$(appdata_DATA);

create-ui-dir: create-data-dir
	if test ! -d $(gtkpod_ui_dir); then \
      sudo mkdir $(gtkpod_ui_dir); \
	fi;

create-data-dir: create-base-data-dir
	if test ! -d $(gtkpod_data_dir); then \
      sudo mkdir $(gtkpod_data_dir); \
	fi;

create-base-data-dir:
	if test ! -d $(datadir)/$(PACKAGE); then \
      sudo mkdir $(datadir)/$(PACKAGE); \
	fi;

# Clean up the links and files created purely for dev testing
clean-local: clean-ui-dir

clean-ui-dir:
	if test -h $(gtkpod_ui_dir)/$(appdata_DATA); then \
      # Symbolic link exists. Removing ..." \
      sudo rm -f $(gtkpod_ui_dir)/$(appdata_DATA); \
	fi;