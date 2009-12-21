message("-------------------------------------------")
message("Configuration started")
message("-------------------------------------------")
project(gtkpod)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

set(PACKAGE ${CMAKE_PROJECT_NAME})
set(GETTEXT_PACKAGE ${CMAKE_PROJECT_NAME})

find_package(PkgConfig REQUIRED)
find_package(GTK2 REQUIRED)
find_package(Glade REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Libgpod REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR}
    ${GTK2_ALL_INCLUDES}
    ${LIBXML2_INCLUDE_DIR}
    ${GLADE2_INCLUDE_DIRS}
    ${LIBGPOD_INCLUDE_DIRS}
)

add_definitions(
    ${GTK2_ALL_CFLAGS}
    ${LIBXML2_DEFINITIONS}
    ${GLADE2_CFLAGS_OTHER}
    ${LIBGPOD_CFLAGS_OTHER}
    -DHAVE_CONFIG_H
)

configure_file(${CMAKE_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_BINARY_DIR}/config.h)

include(cmake/sources.cmake)
add_executable(gtkpod ${GTKPOD_SOURCES})

target_link_libraries(gtkpod
    ${GTK2_ALL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${GLADE2_LDFLAGS}
    ${LIBGPOD_LDFLAGS}
)
