create-glade-dir: create-data-dir
	if test ! -d $(gtkpod_glade_dir); then \
      sudo mkdir $(gtkpod_glade_dir); \
      sudo chmod 755 $(gtkpod_glade_dir); \
	fi;

create-ui-dir: create-data-dir
	if test ! -d $(gtkpod_ui_dir); then \
      sudo mkdir $(gtkpod_ui_dir); \
      sudo chmod 755 $(gtkpod_ui_dir); \
	fi;
	
create-icons-dir: create-data-dir
	if test ! -d $(gtkpod_image_dir); then \
	  sudo mkdir $(gtkpod_image_dir); \
	  sudo chmod 755 $(gtkpod_image_dir); \
	fi;

create-data-dir: create-base-data-dir
	if test ! -d $(gtkpod_data_dir); then \
      sudo mkdir $(gtkpod_data_dir); \
      sudo chmod 755 $(gtkpod_data_dir); \
	fi;

create-base-data-dir:
	if test ! -d $(datadir)/$(PACKAGE); then \
      sudo mkdir $(datadir)/$(PACKAGE); \
      sudo chmod 755 $(datadir)/$(PACKAGE); \
	fi;