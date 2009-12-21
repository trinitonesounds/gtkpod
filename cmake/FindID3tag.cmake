message("-- Checking for id3tag...")
pkg_check_modules(ID3TAG id3tag)

if(NOT ID3TAG_FOUND)
    message(FATAL_ERROR "id3tag not found")
endif()
