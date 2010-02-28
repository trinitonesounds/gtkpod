set(GIO_REQUIRED_VERSION 2.15.0)
message("-- Checking for gio >= ${GIO_REQUIRED_VERSION}...")
pkg_check_modules(GIO gio-2.0>=${GIO_REQUIRED_VERSION})
