set(HAL_REQUIRED_VERSION 0.5)
message("-- Checking for hal >= ${HAL_REQUIRED_VERSION}...")
pkg_check_modules(HAL hal>=${HAL_REQUIRED_VERSION})
