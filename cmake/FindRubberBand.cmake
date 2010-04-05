# FindRubberBand
# Try to find librubberband
#
# Once found, will define:
#
#    RubberBand_FOUND
#    RubberBand_INCLUDE_DIRS
#    RubberBand_LIBRARIES
#

INCLUDE(TritiumPackageHelper)

TPH_FIND_PACKAGE(RubberBand rubberband rubberband/RubberBandStretcher.h rubberband)

INCLUDE(TritiumFindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(RubberBand DEFAULT_MSG RubberBand_LIBRARIES RubberBand_INCLUDE_DIRS)

MARK_AS_ADVANCED(RubberBand_INCLUDE_DIRS RubberBand_LIBRARIES)
