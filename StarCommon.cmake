# Load this cmake file only once
if( StarCommonLoaded )
	message(STATUS "StarCommon: Should be included only once")
	return()
else()
	set(StarCommonLoaded TRUE)
endif()

# By default build shared libraries but allow the user to change if desired
OPTION( BUILD_SHARED_LIBS "Global flag to cause add_library to create shared libraries if on" ON )

# Special treatment of linker options for MacOS X to get a linux-like behavior for gcc
if(APPLE)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

# Set compile warning options for gcc compilers
if( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()


# Make use of the $STAR_HOST_SYS evironment variable. If it is set use it as the
# typical STAR installation prefix
set(STAR_ADDITIONAL_INSTALL_PREFIX ".")

if(DEFINED ENV{STAR_HOST_SYS})
	set(STAR_ADDITIONAL_INSTALL_PREFIX ".$ENV{STAR_HOST_SYS}")
endif()

set( STAR_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" )


#
# Generates a list of header files from which a ROOT dictionary can be created for
# a given subdirectory `star_lib_dir`. The list is put into the `headers_for_dict`
# variable that is returned to the parent scope. Only *.h and *.hh files
# are selected while any LinkDef files are ignored.
#
function(STAR_HEADERS_FOR_ROOT_DICTIONARY star_lib_dir headers_for_dict)

	# Get all header files in 'star_lib_dir'
	file(GLOB_RECURSE star_lib_dir_headers "${star_lib_dir}/*.h"
	                                       "${star_lib_dir}/*.hh")

	# Create an empty list
	set(valid_headers)

	# star_lib_dir_headers should containd absolute paths to globed headers
	foreach( full_path_header ${star_lib_dir_headers} )

		get_filename_component( header_file_name ${full_path_header} NAME )

		string( TOLOWER ${header_file_name} header_file_name )

		# Skip LinkDef files from globbing result
		if( ${header_file_name} MATCHES "linkdef" )
			continue()
		endif()

		list( APPEND valid_headers ${full_path_header} )

	endforeach()

	set( ${headers_for_dict} ${valid_headers} PARENT_SCOPE )

endfunction()


#
# Generates ${star_lib_dir}_LinkDef.h and ${star_lib_dir}_DictInc.h header files
# used in ROOT dictionary generation by rootcint/rootcling. Only header files
# passed in LINKDEF_HEADERS argument are used. The user can optionally pass an
# existing LinkDef file in LINKDEF argument to be incorporated in the generated
# ${star_lib_dir}_LinkDef.h
#
function(STAR_GENERATE_LINKDEF star_lib_dir)
	cmake_parse_arguments(ARG "" "" "LINKDEF;LINKDEF_HEADERS" ${ARGN})

	# Set default name for LinkDef file
	set( linkdef_file "${star_lib_dir}_LinkDef.h" )
	set( dictinc_file "${star_lib_dir}_DictInc.h" )

	# Pass both files to get_likdef.sh as -o arguments
	set( gen_linkdef_args "-l;${linkdef_file};-d;${dictinc_file};${ARG_LINKDEF_HEADERS}" )

	if( ARG_LINKDEF )
		list( APPEND gen_linkdef_args "-i;${ARG_LINKDEF}" )
	endif()

	# Generate the above files to be used in dictionary generation by ROOT
	add_custom_command(
		OUTPUT ${linkdef_file} ${dictinc_file}
		COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_linkdef.sh" ${gen_linkdef_args}
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gen_linkdef.sh" ${ARG_LINKDEF_HEADERS})

endfunction()


#
# Generates a ROOT dictionary for `star_lib_dir` in ${STAR_SRC}.
#
function(STAR_GENERATE_DICTIONARY star_lib_dir star_lib_dir_out)

	cmake_parse_arguments(ARG "" "" "LINKDEF_HEADERS;LINKDEF_OPTIONS;EXCLUDE" "" ${ARGN})

	# Search for default LinkDef if not specified
	file( GLOB user_linkdefs "${star_lib_dir}/*LinkDef.h"
	                         "${star_lib_dir}/*LinkDef.hh" )

	# Get the first LinkDef from the list
	if( user_linkdefs )
		list( GET user_linkdefs 0 user_linkdef )
	endif()

	# If the user provided header files use them in addition to automatically
	# collected ones.
	star_headers_for_root_dictionary( ${star_lib_dir} linkdef_headers )

	FILTER_LIST( linkdef_headers EXCLUDE ${ARG_EXCLUDE} )

	# This is a hack for the call to this function from STAR_ADD_LIBRARY_GEOMETRY() where the
	# headers are generated at runtime and cannot be globbed. So, we can specify them by hand.
	if( NOT linkdef_headers )
		set(linkdef_headers ${ARG_LINKDEF_HEADERS})
	endif()

	# Generate a basic LinkDef file and, if available, merge with the one
	# provided by the user
	star_generate_linkdef( ${star_lib_dir_out} LINKDEF ${user_linkdef} LINKDEF_HEADERS ${linkdef_headers} )

	# Prepare include directories to be used during ROOT dictionary generation.
	# These directories are tied to the `star_lib_dir` traget via the
	# INCLUDE_DIRECTORIES property.
	get_filename_component( star_lib_name ${star_lib_dir} NAME )
	get_target_property( target_include_dirs ${star_lib_name} INCLUDE_DIRECTORIES )
	string( REGEX REPLACE "([^;]+)" "-I\\1" dict_include_dirs "${target_include_dirs}" )

	# May need to look for rootcling first
	find_program(ROOT_DICTGEN_EXECUTABLE rootcint HINTS $ENV{ROOTSYS}/bin)

	# Generate ROOT dictionary using the *_LinkDef.h and *_DictInc.h files
	add_custom_command(OUTPUT ${star_lib_dir_out}_dict.cxx ${star_lib_dir_out}_dict.h
	                   COMMAND ${ROOT_DICTGEN_EXECUTABLE} -cint -f ${star_lib_dir_out}_dict.cxx
	                   -c ${ARG_LINKDEF_OPTIONS} ${dict_include_dirs}
	                   ${ARG_LINKDEF_HEADERS} ${star_lib_dir_out}_DictInc.h ${star_lib_dir_out}_LinkDef.h
	                   DEPENDS ${ARG_LINKDEF_HEADERS} ${star_lib_dir_out}_DictInc.h ${star_lib_dir_out}_LinkDef.h
	                   VERBATIM)
endfunction()


#
# Adds a target to build a library from all source files (*.cxx, *.cc, and *.cpp)
# recursively found in the specified subdirectory `star_lib_dir`. It is possible
# to EXCLUDE some files matching an optional pattern.
#
function(STAR_ADD_LIBRARY star_lib_dir)

	star_target_paths(${star_lib_dir} star_lib_name star_lib_dir_abs star_lib_dir_out)

	# Deal with sources
	set(sources)

	file(GLOB_RECURSE sources_cpp
		"${star_lib_dir_abs}/*.cxx"
		"${star_lib_dir_abs}/*.cc"
		"${star_lib_dir_abs}/*.c"
		"${star_lib_dir_abs}/*.cpp")

	list(APPEND sources ${sources_cpp})

	file(GLOB_RECURSE sources_fortran "${star_lib_dir_abs}/*.F")
	list(APPEND sources ${sources_fortran})

	star_generate_sources_idl(${star_lib_dir_abs} sources_idl)
	list(APPEND sources ${sources_idl})

	GET_EXCLUDE_LIST( ${star_lib_name} star_lib_exclude )
	FILTER_LIST(sources EXCLUDE ${star_lib_exclude})

	# XXX The hardcoded .cxx extension below should be defined by cmake?
	add_library(${star_lib_name} ${sources} ${star_lib_dir_out}_dict.cxx)

	# Output the library to the respecitve subdirectory in the binary directory
	set_target_properties(${star_lib_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${star_lib_dir_out})

	GET_SUBDIRS(${star_lib_dir_abs} star_lib_subdirs INCLUDE_PARENT)
	target_include_directories(${star_lib_name} PRIVATE "${star_lib_subdirs}")

	# Generate the _dict.cxx file for the library
	star_generate_dictionary(${star_lib_dir_abs} ${star_lib_dir_out}
		LINKDEF_HEADERS ${${star_lib_name}_LINKDEF_HEADERS}
		LINKDEF_OPTIONS "-p;-D__ROOT__"
		EXCLUDE ${star_lib_exclude})

	install(TARGETS ${star_lib_name}
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib"
		ARCHIVE DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib")

endfunction()


macro( FILTER_LIST arg_list )

	# Starting cmake 3.6 one can simply use list( FILTER ... )
	#list( FILTER sources EXCLUDE REGEX "${ARG_EXCLUDE}" )

	cmake_parse_arguments(ARG "" "" "EXCLUDE" ${ARGN})

	if( ${arg_list} AND ARG_EXCLUDE )

		foreach( item ${${arg_list}} )
			foreach( regex ${ARG_EXCLUDE} )

				if( ${item} MATCHES "${regex}" )
					list(REMOVE_ITEM ${arg_list} ${item})
					break()
				endif()

			endforeach()
		endforeach()

		set( ${arg_list} ${${arg_list}} )

	endif()

endmacro()


# Return list of regex'es to exclude from globbed paths for target `star_lib_name`
macro(GET_EXCLUDE_LIST star_lib_name exclude_list)
	set( ${exclude_list}
	     ${star_lib_name}.*macros
	     ${star_lib_name}.*doc
	     ${star_lib_name}.*examples
	     ${${star_lib_name}_EXCLUDE} )
endmacro()


# Builds a list of subdirectories with complete path found in the
# 'directories'
macro( GET_SUBDIRS directories subdirectories  )

	cmake_parse_arguments(ARG "INCLUDE_PARENT" "" "" ${ARGN})

	set( sub_dirs "" )

	foreach( dir ${directories} )
		if( NOT IS_ABSOLUTE ${dir})
			set( dir ${STAR_SRC}/${dir})
		endif()

		if( NOT IS_DIRECTORY ${dir} )
			message( WARNING "StarCommon: Directory ${dir} not found" )
			continue()
		endif()

		file( GLOB all_files RELATIVE ${dir} ${dir}/* )

		# Include the parent directory in the list
		if( ${ARG_INCLUDE_PARENT} )
			list( APPEND sub_dirs ${dir} )
		endif()

		foreach( sub_dir ${all_files} )
			if( IS_DIRECTORY ${dir}/${sub_dir} )
				list( APPEND sub_dirs ${dir}/${sub_dir} )
			endif()
		endforeach()

	endforeach()
	
	set( ${subdirectories} ${sub_dirs} )

endmacro()


#
# From `star_lib_dir` extract the library target name and form an absolute and
# a corresponding output paths. The input path `star_lib_dir` can be either
# absolute or relative to ${STAR_SRC}
#
function(STAR_TARGET_PATHS star_lib_dir lib_name path_abs path_out)

	set(path_abs_)
	set(path_out_)

	if( IS_ABSOLUTE ${star_lib_dir} )

		get_filename_component(star_src_abs ${STAR_SRC} ABSOLUTE)
		if( ${star_src_abs} MATCHES ${star_lib_dir} )
			message( FATAL "StarCommon: Absolute path \"${star_lib_dir}\" must match ${star_src_abs}" )
		endif()

		set(path_abs_ ${star_lib_dir})
		file(RELATIVE_PATH path_rel_ ${STAR_SRC} ${star_lib_dir})
		set(path_out_ ${CMAKE_CURRENT_BINARY_DIR}/${path_rel_})
	else()
		set(path_abs_ ${STAR_SRC}/${star_lib_dir})
		set(path_out_ ${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir})
	endif()

	# First check that the path exists
	if( NOT IS_DIRECTORY ${path_abs_} )
		message( FATAL "StarCommon: Directory \"${path_abs_}\" not found" )
	endif()

	get_filename_component(lib_name_ ${star_lib_dir} NAME)

	set(${lib_name} ${lib_name_} PARENT_SCOPE)
	set(${path_abs} ${path_abs_} PARENT_SCOPE)
	set(${path_out} ${path_out_} PARENT_SCOPE)

endfunction()


# special cases
set( St_base_LINKDEF_HEADERS "${STAR_SRC}/StRoot/St_base/Stypes.h" )
set( StEvent_LINKDEF_HEADERS
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2000.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2002.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2003.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2004.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2005.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2007.h"
	"${STAR_CMAKE_DIR}/StArray_cint.h"
)
set( StiMaker_LINKDEF_HEADERS "$ENV{ROOTSYS}/include/TH1K.h" )
set( StEStructPool_LINKDEF_HEADERS
	"$ENV{ROOTSYS}/include/TVector2.h"
	"${STAR_CMAKE_DIR}/StArray_cint.h"
)
set( St_base_EXCLUDE "StRoot/St_base/St_staf_dummies.c" )


#
# Flattens the hierarchy of header files found in `parent_dir` at 1 level deep
# by copying them to `${STAR_ADDITIONAL_INSTALL_PREFIX}/include/`
#
function(STAR_PREINSTALL_HEADERS parent_dir)

	# Get all header files in 'parent_dir'
	file(GLOB header_files
		"${STAR_SRC}/${parent_dir}/*/*.h"
		"${STAR_SRC}/${parent_dir}/*/*.hh")

	foreach( header_file ${header_files})
		get_filename_component( header_file_name ${header_file} NAME )
		configure_file( "${header_file}" "${STAR_ADDITIONAL_INSTALL_PREFIX}/include/${header_file_name}" COPYONLY )
	endforeach()

endfunction()


#
# Creates ROOT TGeo geometries from xml files
#
function(STAR_ADD_LIBRARY_GEOMETRY star_lib_dir)

	# Get first optional unnamed parameter
	set(user_lib_name ${ARGV1})

	star_target_paths(${star_lib_dir} star_lib_name star_lib_dir_abs star_lib_dir_out)

	if( user_lib_name )
		string(REPLACE ${star_lib_name} ${user_lib_name} star_lib_dir_out ${star_lib_dir_out})
		set(star_lib_name ${user_lib_name})
	endif()

	set(geo_py_parser ${STAR_SRC}/mgr/agmlParser.py)

	find_program(EXEC_PYTHON NAMES python2.7 python2)

	file(GLOB_RECURSE geo_xml_paths "${star_lib_dir_abs}/*.xml")
	FILTER_LIST( geo_xml_paths EXCLUDE "Compat" )

	foreach( geo_xml_path ${geo_xml_paths} )

		get_filename_component(geo_name ${geo_xml_path} NAME_WE)

		set(geo_header ${star_lib_dir_out}/${geo_name}.h)
		set(geo_source ${star_lib_dir_out}/${geo_name}.cxx)

		add_custom_command(
			OUTPUT ${geo_header} ${geo_source}
			COMMAND ${EXEC_PYTHON} ${geo_py_parser} --file=${geo_xml_path} --module=${geo_name} --export=AgROOT --path=${star_lib_dir_out}
			DEPENDS ${geo_py_parser} ${geo_xml_path})

		if( ${geo_name} MATCHES "Config")
			list(APPEND geo_config_headers ${geo_header})
		endif()
		list(APPEND geo_sources ${geo_source})

	endforeach()

	# Create a string by replacing ; with gcc compiler options
	string(REGEX REPLACE "([^;]+);" "-include \\1 " geo_config_headers_include "${geo_config_headers};")

	# Special treatment requireed for the aggregate geometry file
	set_source_files_properties(${star_lib_dir_out}/StarGeo.cxx
		PROPERTIES COMPILE_FLAGS "${geo_config_headers_include}")

	add_library(${star_lib_name} ${geo_sources} ${star_lib_dir_out}_dict.cxx)
	set_target_properties(${star_lib_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${star_lib_dir_out})
	target_include_directories(${star_lib_name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

	# Generate the _dict.cxx file for the library
	star_generate_dictionary(${star_lib_dir_out} ${star_lib_dir_out}
		LINKDEF_HEADERS ${star_lib_dir_out}/StarGeo.h
		LINKDEF_OPTIONS "-p;-D__ROOT__")

	install(TARGETS ${star_lib_name}
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib"
		ARCHIVE DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib")

endfunction()


# Generate source from idl files
function(STAR_GENERATE_SOURCES_IDL star_lib_dir sources_idl)

	star_target_paths(${star_lib_dir} star_lib_name star_lib_dir_abs star_lib_dir_out)

	file(GLOB_RECURSE idl_files ${star_lib_dir_abs}/*.idl)

	if( NOT idl_files )
		return()
	endif()

	# For the time being consider only the first file in the list
	list(GET idl_files 0 idll)

	# For the file and variable names we closely follow the convention in mgr/Conscript-standard
	get_filename_component(idl ${idll} NAME_WE)
	set(idlh "${star_lib_dir_out}/${idl}.h")
	set(idli "${star_lib_dir_out}/${idl}.inc")
	set(idlH "${star_lib_dir_out}/St_${idl}_Table.h")
	set(idlC "${star_lib_dir_out}/St_${idl}_Table.cxx")
	set(idlLinkDef "${star_lib_dir_out}/${idl}LinkDef.h")
	set(idlCintH "${star_lib_dir_out}/St_${idl}_TableCint.h")
	set(idlCintC "${star_lib_dir_out}/St_${idl}_TableCint.cxx")

	find_program(STIC_EXECUTABLE stic)

	add_custom_command(
		OUTPUT ${idlh} ${idli}
		COMMAND ${STIC_EXECUTABLE} -q ${idll}
		COMMAND ${CMAKE_COMMAND} -E rename ${idl}.h ${idlh}
		COMMAND ${CMAKE_COMMAND} -E rename ${idl}.inc ${idli}
		DEPENDS ${idll} )

	find_program(PERL_EXECUTABLE perl)

	add_custom_command(
		OUTPUT ${idlH} ${idlC} ${idlLinkDef}
		COMMAND ${PERL_EXECUTABLE} ${STAR_SRC}/mgr/ConstructTable.pl ${idll} ${idlH}
		COMMAND ${PERL_EXECUTABLE} ${STAR_SRC}/mgr/ConstructTable.pl ${idll} ${idlC}
		COMMAND ${PERL_EXECUTABLE} ${STAR_SRC}/mgr/ConstructTable.pl ${idll} ${idlLinkDef}
		DEPENDS ${STAR_SRC}/mgr/ConstructTable.pl ${idll} )

	find_program(ROOT_DICTGEN_EXECUTABLE rootcint HINTS $ENV{ROOTSYS}/bin)

	add_custom_command(OUTPUT ${idlCintC} ${idlCintH}
	                   COMMAND ${ROOT_DICTGEN_EXECUTABLE} -cint -f ${idlCintC} -c -p -D__ROOT__ ${idlh} ${idlH} ${idlLinkDef}
	                   DEPENDS ${idlh} ${idlH} ${idlLinkDef}
	                   VERBATIM)

	set(sources_idl_)
	list(APPEND sources_idl_ ${idlC} ${idlCintC})

	set(${sources_idl} ${sources_idl_} PARENT_SCOPE)

endfunction()
