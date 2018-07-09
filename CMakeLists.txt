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

message(STATUS "star-cmake: CMAKE_CXX_FLAGS = \"${CMAKE_CXX_FLAGS}\"")
message(STATUS "star-cmake: CMAKE_C_FLAGS = \"${CMAKE_C_FLAGS}\"")
message(STATUS "star-cmake: CMAKE_Fortran_FLAGS = \"${CMAKE_Fortran_FLAGS}\"")

star_preinstall_headers(${CMAKE_BINARY_DIR}/include)

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
star_add_library_minicern( StarVMC/minicern StarMiniCern )

star_add_library(pams/geometry)
star_add_library(pams/ctf St_ctf)
star_add_library(pams/sim/g2t St_g2t)

GET_SUBDIRS( "StarVMC;StRoot" star_lib_dirs )
FILTER_LIST( star_lib_dirs EXCLUDE ${STAR_LIB_DIR_BLACKLIST} )

foreach( star_lib_dir ${star_lib_dirs} )
	star_add_library( ${star_lib_dir} )
endforeach()

# Common build rules cannot be applied to StGenericVertexMakerNoSti library
star_add_library_vertexnosti( StRoot/StGenericVertexMaker )
add_dependencies(StGenericVertexMakerNoSti StDb_Tables geometry_Tables sim_Tables)

include("deps.cmake")

# Manually set package specific properties
set_target_properties(TPCCATracker PROPERTIES LINK_LIBRARIES "${ROOT_Vc_LIBRARY}")
target_include_directories(Stv PRIVATE "${STAR_CMAKE_DIR}/star-aux/StarVMC/geant3/TGeant3")


# Copy all header files preserving the subdirectory tree hierarchy
install(DIRECTORY
	${STAR_SRC}/StarVMC
	${STAR_SRC}/StRoot
	DESTINATION ${STAR_ADDITIONAL_INSTALL_PREFIX}/include/
	FILES_MATCHING REGEX "\\.(h|hh)$")

# Copy all generated headers (and the flatten hierarchy of the headers files)
# from the build directory. This is done for backward compatibility when the
# dependent code does not specify the prefix relative to $STAR_SRC in #include
# directives
install(DIRECTORY
	${CMAKE_BINARY_DIR}/include/
	DESTINATION ${STAR_ADDITIONAL_INSTALL_PREFIX}/include_all/
	FILES_MATCHING REGEX "\\.(h|hh)$")

install(DIRECTORY
	${STAR_SRC}/StRoot/StBFChain
	DESTINATION StRoot/)

# Some modules depend on DB entries available on disk
install(DIRECTORY
	${STAR_SRC}/StarDb/Calibrations
	${STAR_SRC}/StarDb/Geometry
	${STAR_SRC}/StarDb/StMagF
	DESTINATION StarDb/)

install(DIRECTORY
	${STAR_SRC}/StDb/servers
	DESTINATION StDb/)
