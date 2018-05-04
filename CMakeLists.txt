cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(star-cvs LANGUAGES C CXX Fortran)

set(STAR_SRC "$ENV{STAR}" CACHE PATH "Path to star software source code")

# Add to path in order to pick up the FindXXX.cmake files included in this project
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

# STAR software depends on ROOT
find_package(ROOT COMPONENTS Table HistPainter Minuit Geom Spectrum Vc minicern)

if( ${ROOT_CXX_FLAGS} MATCHES "-m32" )
	message( STATUS "star-cmake: Found -m32 option in $ROOT_CXX_FLAGS (root-config). Will proceed with 32 bit build" )
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

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

# Remove dependency of "install" target on "all" target. This allows to
# build and install individual libraries
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

# -D_UCMLOGGER_ required by StStarLogger
# -DNEW_DAQ_READER required by StTofHitMaker
set(STAR_C_CXX_DEFINITIONS "-D__ROOT__ -D_UCMLOGGER_ -DNEW_DAQ_READER")
set(STAR_Fortran_DEFINITIONS "-DCERNLIB_TYPE -DCERNLIB_DOUBLE -DCERNLIB_NOQUAD -DCERNLIB_LINUX")
set(STAR_Fortran_FLAGS "-fd-lines-as-code -std=legacy -fno-second-underscore -fno-automatic")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STAR_C_CXX_DEFINITIONS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STAR_C_CXX_DEFINITIONS}")
set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} ${STAR_Fortran_FLAGS} ${STAR_Fortran_DEFINITIONS}")

message(STATUS "star-cmake: CMAKE_CXX_FLAGS = \"${CMAKE_CXX_FLAGS}\"")
message(STATUS "star-cmake: CMAKE_C_FLAGS = \"${CMAKE_C_FLAGS}\"")
message(STATUS "star-cmake: CMAKE_Fortran_FLAGS = \"${CMAKE_Fortran_FLAGS}\"")

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
	${CMAKE_BINARY_DIR}/include
)


star_add_executable_stic(asps/staf/sdd/src)
star_add_executable_agetof(asps/Simulation/agetof)

# Build libraries containing idl/*.idl table descriptions
star_add_library_table( StDb )
star_add_library_table( pams/ctf )
star_add_library_table( pams/emc )
star_add_library_table( pams/ftpc )
star_add_library_table( pams/gen )
star_add_library_table( pams/geometry )
star_add_library_table( pams/global )
star_add_library_table( pams/sim )
star_add_library_table( pams/svt )

# Build library with STAR geometry
star_add_library_geometry( StarVMC/Geometry StarGeometry )

star_add_library(pams/geometry)
star_add_library(pams/ctf St_ctf)
star_add_library(pams/sim/g2t St_g2t)
add_dependencies(St_g2t sim_Tables gen_Tables)

set( star_lib_dir_blacklist
	StarVMC/GeoTestMaker
	StarVMC/minicern
	StarVMC/StarBASE
	StarVMC/StarGeometry      # library built from StarVMC/Geometry
	StarVMC/StarSim
	StarVMC/StarVMCApplication
	StarVMC/StVMCMaker
	StarVMC/StVmcTools
	StarVMC/xgeometry         # library built from StarVMC/Geometry
	StRoot/html
	StRoot/macros
	StRoot/qainfo
	StRoot/StarGenerator
	StRoot/StEEmcPool         # requires subdir processing
	StRoot/StFgtPool          # blacklisted in cons
	StRoot/StFlowMaker        # missing from lib/
	StRoot/St_geom_Maker      # requires qt4/include/QtGui
	StRoot/StHbtMaker         # fortran error
	StRoot/StHighptPool       # blacklisted in cons
	StRoot/StJetMaker
	StRoot/StJetFinder        # needs FindFastJet.cmake
	StRoot/StSpinMaker        # blacklisted due to error in fortran code
	StRoot/StSpinPool         # blacklisted in cons
	StRoot/StTofPool          # missing from lib/
)

GET_SUBDIRS( "StarVMC;StRoot" star_lib_dirs )
FILTER_LIST( star_lib_dirs EXCLUDE ${star_lib_dir_blacklist} )

foreach( star_lib_dir ${star_lib_dirs} )
	star_add_library( ${star_lib_dir} )
	get_filename_component(star_lib_name ${star_lib_dir} NAME)
	add_dependencies(${star_lib_name}
		 St_g2t StDb_Tables emc_Tables gen_Tables ctf_Tables
		 geometry_Tables global_Tables sim_Tables svt_Tables ftpc_Tables)
endforeach()

# Manually set package specific properties
set_target_properties(TPCCATracker PROPERTIES LINK_LIBRARIES "${ROOT_Vc_LIBRARY}")
target_include_directories(Stv PRIVATE "${STAR_CMAKE_DIR}/star-aux/StarVMC/geant3/TGeant3")
target_include_directories(St_g2t PRIVATE
	"${STAR_SRC}/asps/Simulation/geant321/include"
	"${STAR_SRC}/asps/Simulation/starsim/include")

star_add_library_vertexnosti( StRoot/StGenericVertexMaker )
add_dependencies(StGenericVertexMakerNoSti StDb_Tables)
add_dependencies(StEmcCalibrationMaker StEmcADCtoEMaker)
add_dependencies(StEmcPool StEmcADCtoEMaker)
add_dependencies(StEmcRawMaker StEmcADCtoEMaker)
add_dependencies(StGammaMaker StEmcADCtoEMaker)
add_dependencies(StPeCMaker StEmcADCtoEMaker)
add_dependencies(StPreEclMaker StEmcADCtoEMaker)
add_dependencies(StTriggerUtilities StEmcADCtoEMaker)
add_dependencies(St_ctf_Maker St_ctf)


install(DIRECTORY ${STAR_SRC}/StarVMC ${STAR_SRC}/StRoot
	DESTINATION ${STAR_ADDITIONAL_INSTALL_PREFIX}/include
	FILES_MATCHING REGEX "\\.(h|hh)$")

install(DIRECTORY ${STAR_SRC}/StRoot/StBFChain
	DESTINATION StRoot)
