# FindLibSndfile
# Try to find libLibSndfile
#
# Once found, will define:
#
#    LibSndfile_FOUND
#    LibSndfile_INCLUDE_DIRS
#    LibSndfile_LIBRARIES
#

INCLUDE(TritiumPackageHelper)

TPH_FIND_PACKAGE(LibSndfile sndfile sndfile.h sndfile)

INCLUDE(TritiumFindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibSndfile DEFAULT_MSG LibSndfile_LIBRARIES LibSndfile_INCLUDE_DIRS)

MARK_AS_ADVANCED(LibSndfile_INCLUDE_DIRS LibSndfile_LIBRARIES)
