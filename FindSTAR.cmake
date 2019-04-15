# - Find star soft include directories and libraries
#
#  STAR_FOUND
#  STAR_INCLUDE_DIRS
#  STAR_LIBRARIES
#  STAR_LIBRARY_DIRS
#


set(STAR_ROOT "$ENV{STAR}" CACHE STRING "Path to directory with STAR soft installed")

if( NOT STAR_ROOT )
	message(WARNING "STAR_ROOT must be set, i.e. \"cmake -D STAR_ROOT=<path to STAR dir>\" "
	                "Alternatively, one can specify environment variable \"STAR\"")
endif()

# Make use of the $STAR_HOST_SYS evironment variable. If it is set use it as the
# typical STAR installation prefix
set(STAR_ADDITIONAL_INSTALL_PREFIX ".")

if( DEFINED ENV{STAR_HOST_SYS} )
	set(STAR_ADDITIONAL_INSTALL_PREFIX ".$ENV{STAR_HOST_SYS}")
	set(STAR_ROOT "${STAR_ROOT}/${STAR_ADDITIONAL_INSTALL_PREFIX}")
endif()


set(STAR_INCLUDE_DIRS
	# These paths should point to where the STAR soft is installed
	"${STAR_ROOT}/include"
	"${STAR_ROOT}/include/StRoot"
	"${STAR_ROOT}/include/StarVMC"
	"${STAR_ROOT}/include_all"
	# The following is just a workaround for the STAR code design
	# disrespecting the file hierarchy in the installed directory
	"$ENV{STAR}"
	"$ENV{STAR}/StRoot"
	"$ENV{STAR}/StarVMC")

set(STAR_LIBRARY_DIRS "${STAR_ROOT}/lib")

set(star_libs
	StarClassLibrary
	StarMagField
	StarRoot
	StBichsel
	StBTofUtil
	StChain
	StDbBroker
	StDbLib
	StDetectorDbMaker
	StDbUtilities
	StTpcDb
	St_db_Maker
	StEEmcUtil
	StEmcUtil
	StFmsDbMaker
	StFmsUtil
	StEvent
	StEventMaker
	StEventUtilities
	StMcEvent
	StGenericVertexMaker
	Sti
	StiUtilities
	StIOMaker
	StMuDSTMaker
	StStrangeMuDstMaker
	StDb_Tables
	StTableUtilities
	StUtilities
	St_base
	StiIst
	StiTpc
	StiPxl
	StiSsd
	StiSvt
	StiMaker
	StSsdDbMaker
	StSsdUtil
	StSstUtil
	StIstDbMaker
	StarGeometry
	StarAgmlLib
	StarAgmlUtil
	StSvtClassLibrary
	StFtpcTrackMaker
	StFtpcClusterMaker
	St_ctf
	geometry_Tables
	ftpc_Tables
	StDAQMaker
	StDaqLib
	RTS
	StFtpcSlowSimMaker
	StAssociationMaker
	StMcEventMaker
	StarMiniCern
	${STAR_FIND_COMPONENTS}
)

if( star_libs )
	list( REMOVE_DUPLICATES star_libs )
endif()

set(STAR_LIBRARIES)

foreach(star_lib ${star_libs})

	find_library( STAR_LIBRARY_${star_lib} ${star_lib}
	              PATHS ${STAR_LIBRARY_DIRS} )
	
	if( STAR_LIBRARY_${star_lib} )
		mark_as_advanced( STAR_LIBRARY_${star_lib} )
		list( APPEND STAR_LIBRARIES ${STAR_LIBRARY_${star_lib}} )
	endif()

endforeach()


if(NOT WIN32)
	string(ASCII 27 Esc)
	set(ColorReset "${Esc}[m")
	set(ColorGreen "${Esc}[32m")
	set(ColorRed   "${Esc}[31m")
endif()

message(STATUS "Found STAR libraries:")
foreach(star_lib ${star_libs})
	if(STAR_LIBRARY_${star_lib})
	        message(STATUS "  ${ColorGreen}${star_lib}${ColorReset}:\t${STAR_LIBRARY_${star_lib}}")
	else()
	        message(STATUS "  ${ColorRed}${star_lib}${ColorReset}:\t${STAR_LIBRARY_${star_lib}}")
	endif()
endforeach()


mark_as_advanced(STAR_INCLUDE_DIRS STAR_LIBRARIES STAR_LIBRARY_DIRS)


# Set STAR_FOUND to TRUE if all listed variables are TRUE
include( FindPackageHandleStandardArgs )

find_package_handle_standard_args(STAR DEFAULT_MSG
	STAR_INCLUDE_DIRS STAR_LIBRARIES STAR_LIBRARY_DIRS)
