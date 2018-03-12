cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(star-cvs)

set(STAR_SRC "$ENV{STAR}" CACHE PATH "Path to star software source code")

# Add to path in order to pick up the FindXXX.cmake files included in this project
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

# STAR software depends on ROOT
find_package(ROOT COMPONENTS Table HistPainter Minuit Geom Spectrum Vc)

if( ${ROOT_CXX_FLAGS} MATCHES "-m32" )
	message( STATUS "Found -m32 option in $ROOT_CXX_FLAGS (root-config). Will add it to $CMAKE_CXX_FLAGS" )
	set_property( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS FALSE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" )
endif()

find_package(LibXml2)
find_package(Log4Cxx)
find_package(MySQL)

# Perform some setup standard to STAR experiment environment
include(StarCommon)

add_definitions( -D__ROOT__ )
add_definitions( -D_UCMLOGGER_ ) # Required by StStarLogger

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
	StRoot/RTS
	StRoot/StarGenerator
	StRoot/StBTofPool
	StRoot/StEEmcPool
	StRoot/StEmcPool
	StRoot/StEpdHitMaker
	StRoot/StEpdUtil
	StRoot/StEStructPool
	StRoot/StEtrFastSimMaker
	StRoot/StFgtA2CMaker
	StRoot/StFgtClusterMaker
	StRoot/StFgtDbMaker
	StRoot/StFgtPointMaker
	StRoot/StFgtPool
	StRoot/StFgtRawMaker
	StRoot/StFgtSimulatorMaker
	StRoot/StFgtUtil
	StRoot/StFlowMaker
	StRoot/StFpdMaker
	StRoot/StFpsRawHitMaker
	StRoot/StFtpcCalibMaker
	StRoot/StFtpcClusterMaker
	StRoot/StFtpcDriftMapMaker
	StRoot/StFtpcMixerMaker
	StRoot/StFtpcSlowSimMaker
	StRoot/StFtpcTrackMaker
	StRoot/St_geant_Maker
	StRoot/St_geom_Maker
	StRoot/StGridCollector
	StRoot/StHbtMaker
	StRoot/StHighptPool
	StRoot/StHitFilterMaker
	StRoot/StHltMaker
	StRoot/StJetMaker
	StRoot/StRichPool
	StRoot/StSpectraPool
	StRoot/StSpinMaker
	StRoot/StSpinPool
	StRoot/StSvtPool
	StRoot/StTagsMaker
	StRoot/StTofHitMaker
	StRoot/StTofPool
	StRoot/StTrsMaker
)

GET_SUBDIRS( "StarVMC;StRoot" star_lib_dirs )
FILTER_LIST( star_lib_dirs EXCLUDE ${star_lib_dir_blacklist} )

foreach( star_lib_dir ${star_lib_dirs} )
	star_add_library( ${star_lib_dir} )
endforeach()

# Set package specific properties
set_target_properties( TPCCATracker PROPERTIES LINK_LIBRARIES "${ROOT_Vc_LIBRARY}" )


install(DIRECTORY ${STAR_SRC}/StarVMC ${STAR_SRC}/StRoot
        DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/include"
        FILES_MATCHING REGEX "\\.(h|hh)$")
