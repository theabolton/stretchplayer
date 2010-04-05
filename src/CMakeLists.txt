######################################################################
### StretchPlayer Build Script (CMake)                             ###
######################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

######################################################################
### REQUIRED LIBRARIES                                             ###
######################################################################

###
### Qt 4 http://qt.nokia.com/
###

FIND_PACKAGE(Qt4 4.3.0 REQUIRED)
INCLUDE(${QT_USE_FILE})
SET(LIBS ${QT_LIBRARIES})

FIND_PACKAGE(JACK REQUIRED)
INCLUDE(${JACK_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${JACK_LIBRARIES})

FIND_PACKAGE(LibSndfile REQUIRED)
INCLUDE(${LibSndfile_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${LibSndfile_LIBRARIES})

### Don't have a module for rubberband, yet.
SET(LIBS ${LIBS} -lrubberband)

######################################################################
### LIBRARY SOURCES AND BUILD                                      ###
######################################################################

LIST(APPEND sp_cpp
  main.cpp
  PlayerWidget.cpp
  Engine.cpp
  StatusWidget.cpp
  PlayerSizes.cpp
  ThinSlider.cpp
  )

LIST(APPEND sp_hpp
  PlayerWidget.hpp
  Engine.hpp
  StatusWidget.hpp
  PlayerSizes.hpp
  ThinSlider.hpp
  )

LIST(APPEND sp_moc_hpp
  PlayerWidget.hpp
  StatusWidget.hpp
  )

QT4_WRAP_CPP(sp_moc ${sp_moc_hpp}) 
QT4_ADD_RESOURCES(sp_qrc stretchplayer.qrc)

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

ADD_EXECUTABLE(stretchplayer
  ${sp_cpp}
  ${sp_hpp}
  ${sp_moc}
  ${sp_qrc}
  )

TARGET_LINK_LIBRARIES(stretchplayer
    ${LIBS}
    )

INSTALL(TARGETS stretchplayer RUNTIME DESTINATION bin)

######################################################################
### CONFIGURATION SUMMARY                                          ###
######################################################################

MESSAGE("\n"
"Configuration Summary for StretchPlayer\n"
"---------------------------------------\n"
)

MACRO(lib_report name)
  IF(${name}_FOUND)
    message("ENABLED..... ${name}")
  ELSE(${name}_FOUND)
    message("disabled.... ${name}")
  ENDIF(${name}_FOUND)
ENDMACRO(lib_report)

lib_report(QT4)
lib_report(JACK)
lib_report(LibSndfile)

