# TritiumPackageHelper
#
# Defines useful macros for package discovery.
#
# Sets the following:
#
#    TPH_FOUND
#    TPH_FIND_PACKAGE(prefix pkgconfigname header lib)
#
# If the package is found, it will define:
#
#    <prefix>_FOUND
#    <prefix>_INCLUDE_DIRS
#    <prefix>_LIBRARIES
#

SET(TPH_FOUND TRUE)

MACRO(TPH_FIND_PACKAGE prefix pkgconfigname header lib)

  IF(${prefix}_LIBRARIES AND ${prefix}_INCLUDE_DIRS)
    #...already in cache
    SET(${prefix}_FOUND TRUE)
  ELSE(${prefix}_LIBRARIES AND ${prefix}_INCLUDE_DIRS)
    FIND_PACKAGE(PkgConfig)
    IF(PKG_CONFIG_FOUND AND "${pkgconfigname}")
      PKG_SEARCH_MODULE(${prefix} ${pkgconfigname})
    ELSE(PKG_CONFIG_FOUND AND "${pkgconfigname}")
      FIND_PATH(${prefix}_INCLUDE_DIRS ${header})
      FIND_LIBRARY(${prefix}_LIBRARIES ${lib})
      IF(${prefix}_INCLUDE_DIRS AND ${prefix}_LIBRARIES)
	SET(${prefix}_FOUND TRUE)
      ENDIF(${prefix}_INCLUDE_DIRS AND ${prefix}_LIBRARIES)
    ENDIF(PKG_CONFIG_FOUND AND "${pkgconfigname}")

    ## If <prefix>_INCLUDE_DIRS is an empty string,
    ## FIND_PACKAGE_HANDLE_STANDARD_ARGS() chokes.

    IF(${prefix}_FOUND)
      IF(${prefix}_INCLUDE_DIRS STREQUAL "")
	SET(${prefix}_INCLUDE_DIRS /usr/include)
      ENDIF(${prefix}_INCLUDE_DIRS STREQUAL "")
    ENDIF(${prefix}_FOUND)

  ENDIF(${prefix}_LIBRARIES AND ${prefix}_INCLUDE_DIRS)

ENDMACRO(TPH_FIND_PACKAGE)
