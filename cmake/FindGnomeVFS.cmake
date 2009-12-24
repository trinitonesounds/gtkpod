set(GNOME_VFS_REQUIRED_VERSION 2.6.0)
message("-- Checking for gnome-vfs >= ${GNOME_VFS_REQUIRED_VERSION}...")
pkg_check_modules(GNOME_VFS gnome-vfs-2.0>=${GNOME_VFS_REQUIRED_VERSION})
