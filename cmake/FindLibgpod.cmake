set(LIBGPOD_REQUIRED_VERSION 0.7.0)
message("-- Checking for libgpod >= ${LIBGPOD_REQUIRED_VERSION}...")
pkg_check_modules(LIBGPOD libgpod-1.0>=${LIBGPOD_REQUIRED_VERSION})

if(NOT LIBGPOD_FOUND)
    message(FATAL_ERROR "libgpod not found")
endif()
