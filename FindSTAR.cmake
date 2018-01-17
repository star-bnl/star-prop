# - Find star soft include directories and libraries
#
#  STAR_FOUND
#  STAR_INCLUDE_DIRS
#  STAR_LIBRARIES
#  STAR_LIBRARY_DIRS
#

# Perform some setup standard to STAR experiment environment
if( STAR_FIND_REQUIRED_FullSetup )
	include( StarCommon )
	list( REMOVE_ITEM STAR_FIND_COMPONENTS "FullSetup" )
endif()


set( STAR_INCLUDE_DIRS )

find_path( STAR_INCLUDE_DIR_ONE "StChain/StChain.h" PATHS "$ENV{STAR}/StRoot" )
list( APPEND STAR_INCLUDE_DIRS ${STAR_INCLUDE_DIR_ONE})

find_path( STAR_INCLUDE_DIR_TWO "StChain.h" PATHS "$ENV{STAR}/.$ENV{STAR_HOST_SYS}/include" )
list( APPEND STAR_INCLUDE_DIRS ${STAR_INCLUDE_DIR_TWO})


set( STAR_LIBRARY_DIRS "${CMAKE_CURRENT_BINARY_DIR}/.$ENV{STAR_HOST_SYS}/lib"
                       "${STAR_ROOT}.$ENV{STAR_HOST_SYS}/lib"
                       "$ENV{STAR}/.$ENV{STAR_HOST_SYS}/lib" )

set( star_core_libs
	StarClassLibrary
	StarMagField
	StarRoot
	St_base
	StBichsel
	StBTofUtil
	StChain
	StDbBroker
	StDbLib
	StDetectorDbMaker
	St_db_Maker
	StEEmcUtil
	StEmcUtil
	StFmsDbMaker
	StFmsUtil
	StEvent
	StEventMaker
	StGenericVertexMaker
	Sti
	StiUtilities
	StIOMaker
	StMuDSTMaker
	StStrangeMuDstMaker
	St_Tables
	StTableUtilities
	StUtilities
)

set( STAR_LIBRARIES )

foreach( star_component ${star_core_libs} ${STAR_FIND_COMPONENTS} )

	find_library( STAR_${star_component}_LIBRARY ${star_component}
	              PATHS
	              "./.$ENV{STAR_HOST_SYS}/lib"
	              "${STAR_ROOT}.$ENV{STAR_HOST_SYS}/lib"
	              "$ENV{STAR}/.$ENV{STAR_HOST_SYS}/lib"
	)
	
	if( STAR_${star_component}_LIBRARY )
		mark_as_advanced( STAR_${star_component}_LIBRARY )
		list( APPEND STAR_LIBRARIES ${STAR_${star_component}_LIBRARY} )
 		if( STAR_FIND_COMPONENTS )
			list( REMOVE_ITEM STAR_FIND_COMPONENTS ${star_component} )
		endif()
	else()
		message( WARNING "Cannot find STAR component: ${star_component}" )
	endif()

endforeach()

if( STAR_LIBRARIES )
	list( REMOVE_DUPLICATES STAR_LIBRARIES )
endif()


message( STATUS "Found the following STAR libraries:" )
foreach( star_lib ${STAR_LIBRARIES} )
	message(STATUS "  ${star_lib}")
endforeach()


mark_as_advanced( STAR_INCLUDE_DIRS STAR_LIBRARIES STAR_LIBRARY_DIRS )


# Set STAR_FOUND to TRUE if all listed variables are TRUE
include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( STAR DEFAULT_MSG
	STAR_INCLUDE_DIRS STAR_LIBRARIES STAR_LIBRARY_DIRS )
