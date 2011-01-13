######################################################################
### StretchPlayer Build Script (CMake)                             ###
######################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

######################################################################
### INSTALL MISC ART                                               ###
######################################################################

INSTALL(
  FILES stretchplayer-icon-48x48.png
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons
  RENAME stretchplayer.png
  )
INSTALL(
  FILES stretchplayer-icon-48x48.png
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps
  RENAME stretchplayer.png
  )
INSTALL(
  FILES
  stretchplayer-icon.svg
  stretchplayer-icon-16x16.png
  stretchplayer-icon-22x22.png
  stretchplayer-icon-24x24.png
  stretchplayer-icon-32x32.png
  stretchplayer-icon-48x48.png
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/stretchplayer/icons/
  )
