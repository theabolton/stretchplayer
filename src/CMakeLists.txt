######################################################################
### StretchPlayer Build Script (CMake)                             ###
######################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

######################################################################
### CHECK FOR SANE OPTIONS                                         ###
######################################################################

IF( NOT ( AUDIO_SUPPORT_JACK OR AUDIO_SUPPORT_ALSA ) )
  MESSAGE(FATAL_ERROR "Must enable support for at least ONE API")
ENDIF( NOT ( AUDIO_SUPPORT_JACK OR AUDIO_SUPPORT_ALSA ) )

######################################################################
### REQUIRED LIBRARIES                                             ###
######################################################################

###
### Qt 4 http://qt.nokia.com/
###

FIND_PACKAGE(Qt4 4.5.0 REQUIRED)
INCLUDE(${QT_USE_FILE})
SET(LIBS ${QT_LIBRARIES})

IF( AUDIO_SUPPORT_JACK )
  FIND_PACKAGE(JACK REQUIRED)
  INCLUDE_DIRECTORIES(${JACK_INCLUDE_DIRS})
  SET(LIBS ${LIBS} ${JACK_LIBRARIES})
ENDIF( AUDIO_SUPPORT_JACK )

IF( AUDIO_SUPPORT_ALSA )
  FIND_PACKAGE(ALSA REQUIRED)
  INCLUDE_DIRECTORIES(${ALSA_INCLUDE_DIRS})
  SET(LIBS ${LIBS} ${ALSA_LIBRARIES})
ENDIF( AUDIO_SUPPORT_ALSA )

FIND_PACKAGE(LibSndfile REQUIRED)
INCLUDE_DIRECTORIES(${LibSndfile_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${LibSndfile_LIBRARIES})

FIND_PACKAGE(RubberBand REQUIRED)
INCLUDE_DIRECTORIES(${RubberBand_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${RubberBand_LIBRARIES})

######################################################################
### LIBRARY SOURCES AND BUILD                                      ###
######################################################################

LIST(APPEND sp_cpp
  main.cpp
  Configuration.cpp
  PlayerWidget.cpp
  Engine.cpp
  StatusWidget.cpp
  PlayerSizes.cpp
  ThinSlider.cpp
  Marquee.cpp
  AudioSystem.cpp
  jack_memops.c
  bams_format.c
  RubberBandServer.cpp
  )

LIST(APPEND sp_hpp
  Configuration.hpp
  PlayerWidget.hpp
  Engine.hpp
  StatusWidget.hpp
  PlayerSizes.hpp
  ThinSlider.hpp
  Marquee.hpp
  AudioSystem.hpp
  jack_memops.h
  bams_format.h
  RubberBandServer.cpp
  RingBuffer.hpp
  )

LIST(APPEND sp_moc_hpp
  PlayerWidget.hpp
  StatusWidget.hpp
  Marquee.hpp
  )

# Add files for audio API's:

IF( JACK_FOUND )
  LIST(APPEND sp_cpp JackAudioSystem.cpp)
  LIST(APPEND sp_hpp JackAudioSystem.hpp)
ENDIF( JACK_FOUND )

IF( ALSA_FOUND )
  LIST(APPEND sp_cpp AlsaAudioSystem.cpp)
  LIST(APPEND sp_hpp AlsaAudioSystem.hpp)
  LIST(APPEND sp_hpp AlsaAudioSystemPrivate.hpp)
ENDIF( ALSA_FOUND )

QT4_WRAP_CPP(sp_moc ${sp_moc_hpp}) 
QT4_ADD_RESOURCES(sp_qrc stretchplayer.qrc)

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
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
lib_report(ALSA)
lib_report(LibSndfile)
lib_report(RubberBand)
