appdatadir = $(pkgdatadir)/data/ui
appdata_DATA =				\
	gtkpod.ui

EXTRA_DIST = 				\
	$(appdata_DATA)

include ../directories.mk
	
all-local: create-ui-link

# Creating symbolic link to ui file in installed ui directory
create-ui-link: create-ui-dir
	if test -e $(gtkpod_ui_dir)/$(appdata_DATA); then \
      # File already exists. Replacing ..." \
      sudo rm -f $(gtkpod_ui_dir)/$(appdata_DATA); \
	fi; \
    # Creating link for $(appdata_DATA)" \
	sudo ln -s `pwd`/$(appdata_DATA) $(gtkpod_ui_dir)/$(appdata_DATA);

create-ui-dir:
	if test ! -d $(gtkpod_ui_dir); then \
      sudo mkdir -v -p $(gtkpod_ui_dir); \
      sudo chmod 755 $(gtkpod_ui_dir); \
	fi;
	
# Clean up the links and files created purely for dev testing
clean-local: clean-ui-dir

clean-ui-dir:
	if test -h $(gtkpod_ui_dir)/$(appdata_DATA); then \
      # Symbolic link exists. Removing ..." \
      sudo rm -f $(gtkpod_ui_dir)/$(appdata_DATA); \
	fi;