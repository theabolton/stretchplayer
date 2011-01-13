###
### Top directory for StretchPlayer
###

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "503")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")

OPTION(ENABLE_COMPOSITING
  "Enable X11 window compositing, if system supports it."
  ON
  )

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(art)

IF(ENABLE_COMPOSITING)
  SET(USE_COMPOSITING true)
ELSE(ENABLE_COMPOSITING)
  SET(USE_COMPOSITING false)
ENDIF(ENABLE_COMPOSITING)

CONFIGURE_FILE(config.h.in config.h)

CONFIGURE_FILE(stretchplayer.desktop.in stretchplayer.desktop)

ADD_CUSTOM_TARGET(dist
  git archive --format=tar --prefix=stretchplayer-${VERSION}/ HEAD
     | gzip -9
     > ${CMAKE_BINARY_DIR}/stretchplayer_${VERSION}.orig.tar.gz
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Making stretchplayer_${VERSION}.orig.tar.gz"
  )

INSTALL(
  FILES ${CMAKE_BINARY_DIR}/stretchplayer.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications"
  )
INSTALL(
  FILES ${CMAKE_BINARY_DIR}/stretchplayer.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/stretchplayer/"
  )
