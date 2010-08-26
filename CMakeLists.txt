if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message("In-source build attempted; use out-of source builds.")
    message(FATAL_ERROR "Refer to the CMake documentation for more details.")
endif()

message("-------------------------------------------")
message("Configuration started")
message("-------------------------------------------")
project(gtkpod)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

set(PACKAGE ${CMAKE_PROJECT_NAME})
set(VERSION 1.0.0)
set(GETTEXT_PACKAGE ${CMAKE_PROJECT_NAME})
set(PACKAGE_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
set(PACKAGE_LOCALE_DIR ${PACKAGE_DATA_DIR}/locale)

option(USE_CLANG "Use clang instead of gcc to compile" OFF)
option(USE_LOCAL_DATA_FILES
    "Use data files in the source directory when running" OFF)
option(WANT_CURL "Enable build with CURL" ON)
option(WANT_GIO "Enable build with GIO" ON)
option(WANT_LIBVORBISFILE "Enable build with libvorbisfile" ON)
option(WANT_FLAC "Enable build with FLAC" ON)
option(LINK_AS_NEEDED "Link with -Wl,--as-needed" ON)

if(USE_CLANG)
    set(CMAKE_C_COMPILER clang)
endif()

if(USE_LOCAL_DATA_FILES)
    set(PACKAGE_DATA_DIR ${CMAKE_SOURCE_DIR})
endif()

include(flex)
include(CheckIncludeFile)
include(CheckIncludeFiles)

find_package(PkgConfig REQUIRED)
find_package(GTK2 REQUIRED)
find_package(Glade REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Libgpod REQUIRED)
find_package(ID3tag REQUIRED)
CHECK_INCLUDE_FILES(endian.h HAVE_ENDIAN_H)
CHECK_INCLUDE_FILES(mp4v2/platform.h HAVE_MP4V2_PLATFORM_H)

# This is hacky... the result is a test file that looks like this:
# #include <mp4v2/platform.h>
# typedef void* MP4FileHandle;//>
# #include <mp4v2/itmf_tags.h>
# This matches the way it's used in the code, and the autoconf test
CHECK_INCLUDE_FILES("mp4v2/platform.h>\ntypedef void* MP4FileHandle\;//;mp4v2/itmf_tags.h" HAVE_MP4V2_ITMF_TAGS_H)

set(OPT_INCLUDES "")
set(OPT_CFLAGS "")
set(OPT_LIBS "")

if(WANT_CURL)
    message("-- Checking for CURL...")
    find_package(CURL)
    if(CURL_FOUND)
        set(HAVE_CURL ON)
        set(OPT_INCLUDES ${OPT_INCLUDES} ${CURL_INCLUDE_DIRS})
        set(OPT_LIBS ${OPT_LIBS} ${CURL_LIBRARIES})
    endif()
endif()

if(WANT_GIO)
    find_package(GIO)
    if(GIO_FOUND)
        set(HAVE_GIO 1)
        set(OPT_INCLUDES ${OPT_INCLUDES} ${GIO_INCLUDE_DIRS})
        set(OPT_CFLAGS ${OPT_CFLAGS} ${GIO_CFLAGS_OTHER})
        set(OPT_LIBS ${OPT_LIBS} ${GIO_LDFLAGS})
    endif()
endif()

if(WANT_LIBVORBISFILE)
    find_package(VorbisFile)
    if(VORBISFILE_FOUND)
        set(HAVE_LIBVORBISFILE 1)
        set(OPT_INCLUDES ${OPT_INCLUDES} ${VORBISFILE_INCLUDE_DIRS})
        set(OPT_CFLAGS ${OPT_CFLAGS} ${VORBISFILE_CFLAGS_OTHER})
        set(OPT_LIBS ${OPT_LIBS} ${VORBISFILE_LDFLAGS})
    endif()
endif()

if(WANT_FLAC)
    find_package(FLAC)
    if(FLAC_FOUND)
        set(HAVE_FLAC 1)
        set(OPT_INCLUDES ${OPT_INCLUDES} ${FLAC_INCLUDE_DIRS})
        set(OPT_CFLAGS ${OPT_CFLAGS} ${FLAC_CFLAGS_OTHER})
        set(OPT_LIBS ${OPT_LIBS} ${FLAC_LDFLAGS})
    endif()
endif()

if(LINK_AS_NEEDED)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
endif()

check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(stdint.h HAVE_STDINT_H)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${GTK2_ALL_INCLUDES}
    ${LIBXML2_INCLUDE_DIR}
    ${GLADE2_INCLUDE_DIRS}
    ${LIBGPOD_INCLUDE_DIRS}
    ${ID3TAG_INCLUDE_DIRS}
    ${OPT_INCLUDES}
)

add_definitions(
    ${GTK2_ALL_CFLAGS}
    ${LIBXML2_DEFINITIONS}
    ${GLADE2_CFLAGS_OTHER}
    ${LIBGPOD_CFLAGS_OTHER}
    ${ID3TAG_CFLAGS_OTHER}
    ${OPT_CFLAGS}
    -DHAVE_CONFIG_H
)

configure_file(${CMAKE_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_BINARY_DIR}/config.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/data/man/gtkpod.1.in
    ${CMAKE_BINARY_DIR}/gtkpod.1 @ONLY)

set(INTLTOOL_MERGE_EXECUTABLE ${CMAKE_SOURCE_DIR}/cmake/intltool-merge)

add_custom_target(desktopfile ALL ${INTLTOOL_MERGE_EXECUTABLE} -d
    ${CMAKE_SOURCE_DIR}/po ${CMAKE_SOURCE_DIR}/data/gtkpod.desktop.in
    ${CMAKE_BINARY_DIR}/gtkpod.desktop)

include(sources)
include(Po)
compile_po_files(po GTKPOD_SOURCES)
add_executable(gtkpod ${GTKPOD_SOURCES})

target_link_libraries(gtkpod
    ${GTK2_ALL_LIBS}
    ${LIBXML2_LIBRARIES}
    ${GLADE2_LDFLAGS}
    ${LIBGPOD_LDFLAGS}
    ${ID3TAG_LDFLAGS}
    ${OPT_LIBS}
)

include(install)
