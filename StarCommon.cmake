# Load this cmake file only once
if( StarCommonLoaded )
	message(STATUS "StarCommon: Should be included only once")
	return()
else()
	set(StarCommonLoaded TRUE)
endif()

# By default build shared libraries but allow the user to change if desired
OPTION( BUILD_SHARED_LIBS "Global flag to cause add_library to create shared libraries if on" ON )

# Special treatment of linker options for MacOS X to get a gcc linux-like behavior
if(APPLE)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

# Set compile warning options for gcc compilers
if( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()


# Check whether the compiler supports c++11
include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(STATUS "StarCommon: The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


message(STATUS "StarCommon: CMAKE_CXX_FLAGS = \"${CMAKE_CXX_FLAGS}\"")


# Remove dependency of "install" target on "all" target. This allows to
# build and install individual libraries
set( CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE )


# Make use of the $STAR_HOST_SYS evironment variable. If it is set use it as the
# typical STAR installation prefix
set(STAR_ADDITIONAL_INSTALL_PREFIX ".")

if(DEFINED ENV{STAR_HOST_SYS})
	set(STAR_ADDITIONAL_INSTALL_PREFIX ".$ENV{STAR_HOST_SYS}")
endif()

set( STAR_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" )


#
# Builds a list of header files from which a ROOT dictionary can be created for
# a given subdirectory `star_lib_dir`. The list is put into the `headers_for_dict`
# variable that is returned to the parent scope. Only *.h and *.hh files
# are selected while any LinkDef files are ignored.
#
function( STAR_HEADERS_FOR_ROOT_DICTIONARY star_lib_dir headers_for_dict )

	# Get all header files in 'star_lib_dir'
	file(GLOB_RECURSE star_lib_dir_headers "${STAR_SRC}/${star_lib_dir}/*.h"
	                                       "${STAR_SRC}/${star_lib_dir}/*.hh")

	# Create an empty list
	set(valid_headers)

	# star_lib_dir_headers should containd absolute paths to globed headers
	foreach( full_path_header ${star_lib_dir_headers} )

		get_filename_component( header_file_name ${full_path_header} NAME )

		string( TOLOWER ${header_file_name} header_file_name )

		# Skip LinkDef files from globbing result
		if( ${header_file_name} MATCHES "linkdef" )
			# Uncomment next line to make it verbose
			#message( STATUS "StarCommon: WARNING: Skipping LinkDef header ${full_path_header}" )
			continue()
		endif()

		list( APPEND valid_headers ${full_path_header} )

	endforeach()

	set( ${headers_for_dict} ${valid_headers} PARENT_SCOPE )

endfunction()


#
# Generate an automatic LinkDef header ${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_LinkDef.h
#
function(STAR_GENERATE_LINKDEF star_lib_dir)
	cmake_parse_arguments(ARG "" "" "LINKDEF;LINKDEF_HEADERS" ${ARGN})

	# Set default name for LinkDef file
	set( linkdef_file "${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_LinkDef.h" )
	set( dictinc_file "${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_DictInc.h" )

	# Pass both files to get_likdef.sh as -o arguments
	set( gen_linkdef_args "-o;${linkdef_file} ${dictinc_file};${ARG_LINKDEF_HEADERS}" )

	if( ARG_LINKDEF )
		list( APPEND gen_linkdef_args "-l;${ARG_LINKDEF}" )
	endif()

	# Generate the above files to be used in dictionary generation by ROOT
	add_custom_command(
		OUTPUT ${linkdef_file} ${dictinc_file}
		COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_linkdef.sh" ${gen_linkdef_args}
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gen_linkdef.sh" ${ARG_LINKDEF_HEADERS}
		)
endfunction()


#
# Generates a ROOT dictionary for `star_lib_dir`.
#
function(STAR_GENERATE_DICTIONARY star_lib_dir)

	cmake_parse_arguments(ARG "" "" "LINKDEF;LINKDEF_HEADERS;LINKDEF_OPTIONS;EXCLUDE" "" ${ARGN})

	# If the user provided header files use them in addition to automatically
	# collected ones.
	set( linkdef_headers )
	star_headers_for_root_dictionary( ${star_lib_dir} linkdef_headers )

	if( ARG_EXCLUDE )
		FILTER_LIST( linkdef_headers "${ARG_EXCLUDE}" )
	endif()

	# Generate a basic LinkDef file and, if available, merge with the one
	# provided by the user
	star_generate_linkdef( ${star_lib_dir} LINKDEF ${ARG_LINKDEF} LINKDEF_HEADERS ${linkdef_headers})

	root_generate_dictionary( ${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_dict
		${ARG_LINKDEF_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_DictInc.h
		LINKDEF ${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_LinkDef.h
		OPTIONS ${ARG_LINKDEF_OPTIONS}
	)

endfunction()


#
# Adds a target to build a library from all source files (*.cxx, *.cc, and *.cpp)
# recursively found in the specified subdirectory `star_lib_dir`. It is possible
# to EXCLUDE some files matching an optional pattern.
#
function(STAR_ADD_LIBRARY star_lib_dir)

	# First check that the path exists
	if( NOT IS_DIRECTORY ${STAR_SRC}/${star_lib_dir} )
		message( WARNING "StarCommon: Subdirectory \"${star_lib_dir}\" not found in ${STAR_SRC}. Skipping" )
		return()
	endif()

	get_filename_component( star_lib_name ${star_lib_dir} NAME )

	cmake_parse_arguments(ARG "" "" "LINKDEF;LINKDEF_HEADERS;LINKDEF_OPTIONS;EXCLUDE" "" ${ARGN})

	# Set default regex'es to exclude from globbed
	list( APPEND ARG_EXCLUDE "${star_lib_dir}.*macros;${star_lib_dir}.*doc;${star_lib_dir}.*examples" )

	# Deal with headers

	# Search for default LinkDef if not specified
	file( GLOB user_linkdefs "${STAR_SRC}/${star_lib_dir}/*LinkDef.h"
	                         "${STAR_SRC}/${star_lib_dir}/*LinkDef.hh" )

	if( NOT ARG_LINKDEF AND user_linkdefs )
		# Get the first LinkDef from the list
		list( GET user_linkdefs 0 user_linkdef )
		set( ARG_LINKDEF ${user_linkdef} )
	endif()

	# Set default options
	list(APPEND ARG_LINKDEF_OPTIONS "-p;-D__ROOT__" )

	star_generate_dictionary( ${star_lib_dir}
		LINKDEF ${ARG_LINKDEF}
		LINKDEF_HEADERS ${ARG_LINKDEF_HEADERS} ${${star_lib_name}_LINKDEF_HEADERS}
		LINKDEF_OPTIONS ${ARG_LINKDEF_OPTIONS}
		EXCLUDE ${ARG_EXCLUDE}
	)

	# Deal with sources
	file(GLOB_RECURSE sources "${STAR_SRC}/${star_lib_dir}/*.cxx"
	                          "${STAR_SRC}/${star_lib_dir}/*.cc"
	                          "${STAR_SRC}/${star_lib_dir}/*.cpp")

	if( ARG_EXCLUDE )
		FILTER_LIST( sources "${ARG_EXCLUDE}" )
	endif()

	add_library(${star_lib_name} ${sources} ${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_dict.cxx)

	# Output the library to the respecitve subdirectory in the binary directory
	get_filename_component( star_lib_path ${star_lib_dir} DIRECTORY )
	set_target_properties( ${star_lib_name} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${star_lib_path}" )

	get_subdirs( ${STAR_SRC}/${star_lib_dir} star_lib_subdirs )

	target_include_directories( ${star_lib_name} PRIVATE "${star_lib_subdirs}" )

	install( TARGETS ${star_lib_name}
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib"
		ARCHIVE DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib"
	)

endfunction()


function( FILTER_LIST arg_list arg_regexs )

	# Starting cmake 3.6 one can simply use list( FILTER ... )
	#list( FILTER sources EXCLUDE REGEX "${ARG_EXCLUDE}" )

	foreach( item ${${arg_list}} )
		foreach( regex ${arg_regexs} )

			if( ${item} MATCHES "${regex}" )
				list(REMOVE_ITEM ${arg_list} ${item})
				break()
			endif()

		endforeach()
	endforeach()

	set( ${arg_list} ${${arg_list}} PARENT_SCOPE)

endfunction()


# Builds a list of subdirectories with complete path found in the
# 'parent_directory'
macro( GET_SUBDIRS parent_directory subdirectories  )

	file( GLOB all_files RELATIVE ${parent_directory} ${parent_directory}/* )

	# Include the parent directory in the list
	set( sub_dirs "${parent_directory}" )
	
	foreach( sub_dir ${all_files} )
		if( IS_DIRECTORY ${parent_directory}/${sub_dir} )
			list( APPEND sub_dirs ${parent_directory}/${sub_dir} )
		endif()
	endforeach()
	
	set( ${subdirectories} ${sub_dirs} )

endmacro()


# special cases
set( St_base_LINKDEF_HEADERS "${STAR_SRC}/StRoot/St_base/Stypes.h" )
set( StEvent_LINKDEF_HEADERS
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2000.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2002.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2003.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2004.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2005.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2007.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures.h"
	"${STAR_CMAKE_DIR}/StArray_cint.h"
)
set( StiMaker_LINKDEF_HEADERS "$ENV{ROOTSYS}/include/TH1K.h" )


#
# Flattens the hierarchy of header files found in `parent_dir` at 1 level deep
# by copying them to `${STAR_ADDITIONAL_INSTALL_PREFIX}/include/`
#
function( STAR_PREINSTALL_HEADERS parent_dir )

	# Get all header files in 'parent_dir'
	file( GLOB header_files "${STAR_SRC}/${parent_dir}/*/*.h"
	                        "${STAR_SRC}/${parent_dir}/*/*.hh" )


	foreach( header_file ${header_files})
		get_filename_component( header_file_name ${header_file} NAME )
		configure_file( "${header_file}" "${STAR_ADDITIONAL_INSTALL_PREFIX}/include/${header_file_name}" COPYONLY )
	endforeach()

endfunction()
