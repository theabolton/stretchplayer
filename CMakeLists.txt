###
### Top directory for StretchPlayer
###

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "504")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")

OPTION(AUDIO_SUPPORT_JACK
  "Enable support for JACK audio API"
  ON
  )

OPTION(AUDIO_SUPPORT_ALSA
  "Enable support for ALSA audio API"
  ON
  )

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(art)

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
