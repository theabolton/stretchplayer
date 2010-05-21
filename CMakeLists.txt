###
### Top directory for StretchPlayer
###

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "501")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(art)

CONFIGURE_FILE(config.h.in config.h)

CONFIGURE_FILE(stretchplayer.desktop.in stretchplayer.desktop)
INSTALL(
  FILES ${CMAKE_BINARY_DIR}/stretchplayer.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications"
  )
INSTALL(
  FILES ${CMAKE_BINARY_DIR}/stretchplayer.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/streatchplayer/"
  )
