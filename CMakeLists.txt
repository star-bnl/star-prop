cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(star-cvs LANGUAGES C CXX Fortran)

set(STAR_SRC "$ENV{STAR}" CACHE PATH "Path to star software source code")

# Add to path in order to pick up the FindXXX.cmake files included in this project
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

# STAR software depends on ROOT
find_package(ROOT COMPONENTS Table HistPainter Minuit Geom Spectrum Vc)

if( ${ROOT_CXX_FLAGS} MATCHES "-m32" )
	message( STATUS "Found -m32 option in $ROOT_CXX_FLAGS (root-config). Will add it to $CMAKE_CXX_FLAGS" )
	set_property( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS FALSE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32" )
	set( CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -m32" )
endif()

find_package(LibXml2)
find_package(Log4Cxx)
find_package(MySQL)

# Perform some setup standard to STAR experiment environment
include(StarCommon)

add_definitions( -D__ROOT__ )
add_definitions( -D_UCMLOGGER_ ) # Required by StStarLogger
add_definitions( -DNEW_DAQ_READER ) # Required by StTofHitMaker

# Remove dependency of "install" target on "all" target. This allows to
# build and install individual libraries
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

star_preinstall_headers( StarVMC )
star_preinstall_headers( StRoot )

include_directories(
	${ROOT_INCLUDE_DIR}
	${LIBXML2_INCLUDE_DIR}
	${LOG4CXX_INCLUDE_DIR}
	${MYSQL_INCLUDE_DIRS}
	${STAR_SRC}
	${STAR_SRC}/StRoot
	${STAR_SRC}/StRoot/RTS/include
	${STAR_SRC}/StRoot/RTS/src
	${STAR_SRC}/StRoot/RTS/trg/include
	# Inlude path to header files with flatten directory hierarchy
	# created by star_preinstall_headers()
	${CMAKE_BINARY_DIR}/${STAR_ADDITIONAL_INSTALL_PREFIX}/include
	# The following directories need to be included until header files can
	# be generated from idl
	$ENV{STAR}/.$ENV{STAR_HOST_SYS}/include/tables
	$ENV{STAR}/.$ENV{STAR_HOST_SYS}/include
)

set( star_lib_dir_blacklist
	StarVMC/GeoTestMaker
	StarVMC/minicern
	StarVMC/StarBASE
	StarVMC/StarGeometry
	StarVMC/StarSim
	StarVMC/StarVMCApplication
	StarVMC/StVMCMaker
	StarVMC/StVmcTools
	StarVMC/xgeometry
	StRoot/html
	StRoot/macros
	StRoot/qainfo
	StRoot/StarGenerator
	StRoot/StEEmcPool
	StRoot/StFgtPool          # blacklisted in cons
	StRoot/StFlowMaker
	StRoot/St_geant_Maker
	StRoot/St_geom_Maker
	StRoot/StHighptPool       # blacklisted in cons
	StRoot/StJetMaker
	StRoot/StSpinMaker        # blacklisted due to error in fortran code
	StRoot/StSpinPool         # blacklisted in cons
	StRoot/StTagsMaker
	StRoot/StTofPool
	StRoot/StTagsMaker        # idl
)

GET_SUBDIRS( "StarVMC;StRoot" star_lib_dirs )
FILTER_LIST( star_lib_dirs EXCLUDE ${star_lib_dir_blacklist} )

foreach( star_lib_dir ${star_lib_dirs} )
	star_add_library( ${star_lib_dir} )
endforeach()

# Set package specific properties
set_target_properties( TPCCATracker PROPERTIES LINK_LIBRARIES "${ROOT_Vc_LIBRARY}" )

# Build library with STAR geometry
star_add_library_geometry( StarVMC/Geometry StarGeometry )


install(DIRECTORY ${STAR_SRC}/StarVMC ${STAR_SRC}/StRoot
        DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/include"
        FILES_MATCHING REGEX "\\.(h|hh)$")
