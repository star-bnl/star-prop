# - Find cernlib include directories and libraries
#
#  CERNLIB_FOUND
#  CERNLIB_INCLUDE_DIR
#  CERNLIB_LIBRARIES
#  CERNLIB_LIBRARY_DIR

get_property(USE_LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if(${USE_LIB64})
	set(CERNLIB_LIBRARY_DIR "/cern64/pro/lib/")
	set(CERNLIB_INCLUDE_DIR "/cern64/pro/include/")
else()
	set(CERNLIB_LIBRARY_DIR "/cern/pro/lib/")
	set(CERNLIB_INCLUDE_DIR "/cern/pro/include/")
endif()

set(cernlibs ${CERNLIB_FIND_COMPONENTS})

set(CERNLIB_LIBRARIES )
foreach(cernlib ${cernlibs})
	find_library(CERNLIB_LIBRARY_${cernlib} ${cernlib} PATHS ${CERNLIB_LIBRARY_DIR} NO_DEFAULT_PATH)
	list(APPEND CERNLIB_LIBRARIES ${CERNLIB_LIBRARY_${cernlib}})
endforeach()

if(NOT WIN32)
	string(ASCII 27 Esc)
	set(ColorReset "${Esc}[m")
	set(ColorGreen "${Esc}[32m")
	set(ColorRed   "${Esc}[31m")
endif()

message(STATUS "Found CERNLIB libraries:")
foreach(cernlib ${cernlibs})
	if(CERNLIB_LIBRARY_${cernlib})
		message(STATUS "  ${ColorGreen}${cernlib}${ColorReset}:\t${CERNLIB_LIBRARY_${cernlib}}")
	else()
		message(STATUS "  ${ColorRed}${cernlib}${ColorReset}:\t${CERNLIB_LIBRARY_${cernlib}}")
	endif()
endforeach()

mark_as_advanced(CERNLIB_INCLUDE_DIR CERNLIB_LIBRARIES CERNLIB_LIBRARY_DIR)

# Set CERNLIB_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(CERNLIB DEFAULT_MSG
	CERNLIB_INCLUDE_DIR CERNLIB_LIBRARIES CERNLIB_LIBRARY_DIR)
