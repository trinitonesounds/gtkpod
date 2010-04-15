appdatadir = $(pkgdatadir)/data/ui
appdata_gtkpod = gtkpod.ui
appdata_preferences = preferences.ui

EXTRA_DIST = 				\
	$(appdata_gtkpod) \
	$(appdata_preferences)

include ../directories.mk
	
all-local: create-gtkpod-ui-link create-preferences-ui-link

# Creating symbolic link to ui file in installed ui directory
create-gtkpod-ui-link: create-ui-dir
	if test -e $(gtkpod_ui_dir)/$(appdata_gtkpod); then \
      # File already exists. Replacing ..." \
      sudo rm -f $(gtkpod_ui_dir)/$(appdata_gtkpod); \
	fi; \
    # Creating link for $(appdata_gtkpod)" \
	sudo ln -s `pwd`/$(appdata_gtkpod) $(gtkpod_ui_dir)/$(appdata_gtkpod);

create-preferences-ui-link: create-ui-dir
	if test -e $(gtkpod_ui_dir)/$(appdata_preferences); then \
      # File already exists. Replacing ..." \
      sudo rm -f $(gtkpod_ui_dir)/$(appdata_preferences); \
	fi; \
    # Creating link for $(appdata_preferences)" \
	sudo ln -s `pwd`/$(appdata_preferences) $(gtkpod_ui_dir)/$(appdata_preferences);

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