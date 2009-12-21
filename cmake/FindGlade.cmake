set(GLADE_REQUIRED_VERSION 2.4.0)
message("-- Checking for libglade >= ${GLADE_REQUIRED_VERSION}...")
pkg_check_modules(GLADE2 libglade-2.0>=${GLADE_REQUIRED_VERSION})

if(NOT GLADE2_FOUND)
    message(FATAL_ERROR "libglade not found")
endif()
