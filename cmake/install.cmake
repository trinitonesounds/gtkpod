set(DATA_INSTALL_DEST ${PACKAGE_DATA_DIR}/${CMAKE_PROJECT_NAME})
install(TARGETS gtkpod DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(FILES
    data/default-cover.png
    data/gtkpod-add-dirs.png
    data/gtkpod-add-files.png
    data/gtkpod-add-playlists.png
    data/gtkpod-icon-32-2.png
    data/gtkpod-icon-32.png
    data/gtkpod-icon-48.png
    data/gtkpod-logo.png
    data/gtkpod-read-16.png
    data/gtkpod-read.png
    data/gtkpod-sync.png
    data/photo-toolbar-album.png
    data/photo-toolbar-photos.png
    data/gtkpod.glade
    DESTINATION ${DATA_INSTALL_DEST}/data)

install(DIRECTORY data/ui DESTINATION ${DATA_INSTALL_DEST}/data)
install(DIRECTORY doc scripts DESTINATION ${DATA_INSTALL_DEST})
install(FILES ${CMAKE_BINARY_DIR}/gtkpod.1 DESTINATION ${PACKAGE_DATA_DIR}/man/man1)
install(FILES ${CMAKE_BINARY_DIR}/gtkpod.desktop DESTINATION ${PACKAGE_DATA_DIR}/applications)
install(DIRECTORY data/icons/hicolor DESTINATION ${DATA_INSTALL_DEST}/icons)

install(FILES data/icons/16x16/gtkpod.png DESTINATION ${PACKAGE_DATA_DIR}/icons/hicolor/16x16/apps)
install(FILES data/icons/22x22/gtkpod.png DESTINATION ${PACKAGE_DATA_DIR}/icons/hicolor/22x22/apps)
install(FILES data/icons/24x24/gtkpod.png DESTINATION ${PACKAGE_DATA_DIR}/icons/hicolor/24x24/apps)
install(FILES data/icons/32x32/gtkpod.png DESTINATION ${PACKAGE_DATA_DIR}/icons/hicolor/32x32/apps)
install(FILES data/icons/48x48/gtkpod.png DESTINATION ${PACKAGE_DATA_DIR}/icons/hicolor/48x48/apps)
install(FILES data/icons/64x64/gtkpod.png DESTINATION ${PACKAGE_DATA_DIR}/icons/hicolor/64x64/apps)
install(FILES data/icons/scalable/gtkpod.svg DESTINATION ${PACKAGE_DATA_DIR}/icons/hicolor/scalable/apps)
