# - Find cernlib include directories and libraries
#
#  CERNLIB_FOUND
#  CERNLIB_INCLUDE_DIRS
#  CERNLIB_LIBRARIES

set(CERNLIB_ROOT "${CERNLIB_ROOT}" CACHE PATH "CERNLIB root directory to look in")

if(DEFINED ENV{CERN_ROOT})
	set(CERNLIB_ROOT "$ENV{CERN_ROOT}" CACHE PATH "CERNLIB root directory to look in" FORCE)
endif()

message(STATUS "Searching for CERNLIB components in ${CERNLIB_ROOT}")

set(_cernlibs ${CERNLIB_FIND_COMPONENTS})

# Define colors for nicer output to the terminal
if(NOT WIN32)
	string(ASCII 27 Esc)
	set(ColorReset "${Esc}[m")
	set(ColorGreen "${Esc}[32m")
	set(ColorRed   "${Esc}[31m")
endif()

set(CERNLIB_LIBRARIES)
foreach(_cernlib ${_cernlibs})

	set(_cernlib_names "${_cernlib}")

	# Search for lapack under a different name. This is relevant for STAR
	# experiment setup where lapack3 comes from CERNLIB_ROOT location
	if(_cernlib STREQUAL "lapack")
		set(_cernlib_names "lapack" "lapack3")
	endif()

	find_library(CERNLIB_${_cernlib}_LIBRARY
		NAMES ${_cernlib_names}
		PATHS "${CERNLIB_ROOT}"
		PATH_SUFFIXES lib)

	if(CERNLIB_${_cernlib}_LIBRARY)
		set(CERNLIB_${_cernlib}_FOUND TRUE)
		message(STATUS "  ${ColorGreen}${_cernlib}${ColorReset}:\t${CERNLIB_${_cernlib}_LIBRARY}")
	else()
		set(CERNLIB_${_cernlib}_FOUND FALSE)
		message(STATUS "  ${ColorRed}${_cernlib}${ColorReset}:\t${CERNLIB_${_cernlib}_LIBRARY}")
	endif()

	list(APPEND CERNLIB_LIBRARIES ${CERNLIB_${_cernlib}_LIBRARY})
endforeach()

# Although not checked explicitly the hope is that the other header files (e.g.
# paw/paw.h) are in the same parent directory
find_path(CERNLIB_INCLUDE_DIRS
	NAMES "cfortran/cfortran.h"
	PATHS "${CERNLIB_ROOT}"
	PATH_SUFFIXES include)

# Set CERNLIB_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CERNLIB REQUIRED_VARS CERNLIB_INCLUDE_DIRS HANDLE_COMPONENTS)

mark_as_advanced(CERNLIB_ROOT CERNLIB_LIBRARIES CERNLIB_INCLUDE_DIRS)
