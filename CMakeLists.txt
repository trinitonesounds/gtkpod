message("-------------------------------------------")
message("Configuration started")
message("-------------------------------------------")
project(gtkpod)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

set(PACKAGE ${CMAKE_PROJECT_NAME})
set(VERSION 0.99.14)
set(GETTEXT_PACKAGE ${CMAKE_PROJECT_NAME})
set(PACKAGE_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
set(PACKAGE_LOCALE_DIR ${PACKAGE_DATA_DIR}/${PACKAGE}/locale)

option(USE_CLANG "OFF")
option(USE_LOCAL_DATA_FILES "OFF")

if(USE_CLANG)
    set(CMAKE_C_COMPILER clang)
endif()

if(USE_LOCAL_DATA_FILES)
    set(PACKAGE_DATA_DIR ${CMAKE_SOURCE_DIR})
endif()

message(${PACKAGE_DATA_DIR})
include(flex)
include(CheckIncludeFile)

find_package(PkgConfig REQUIRED)
find_package(GTK2 REQUIRED)
find_package(Glade REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Libgpod REQUIRED)
find_package(ID3tag REQUIRED)

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
)

add_definitions(
    ${GTK2_ALL_CFLAGS}
    ${LIBXML2_DEFINITIONS}
    ${GLADE2_CFLAGS_OTHER}
    ${LIBGPOD_CFLAGS_OTHER}
    ${ID3TAG_CFLAGS_OTHER}
    -DHAVE_CONFIG_H
)

configure_file(${CMAKE_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_BINARY_DIR}/config.h)

include(cmake/sources.cmake)
add_executable(gtkpod ${GTKPOD_SOURCES})

target_link_libraries(gtkpod
    ${GTK2_ALL_LIBS}
    ${LIBXML2_LIBRARIES}
    ${GLADE2_LDFLAGS}
    ${LIBGPOD_LDFLAGS}
    ${ID3TAG_LDFLAGS}
)
