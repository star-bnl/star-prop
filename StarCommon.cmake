# Load this cmake file only once
if( StarCommonLoaded )
	message(STATUS "StarCommon: Should be included only once")
	return()
else()
	set(StarCommonLoaded TRUE)
endif()


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


# STAR software heavily depends on ROOT
find_package( ROOT COMPONENTS Table HistPainter Minuit Geom Spectrum )

# Since most of STAR projects depend on ROOT check the flags and use the same
if(ROOT_FOUND)

	string(REGEX MATCH "(^|[\t ]+)-m([\t ]*)(32|64)([\t ]+|$)" STAR_ROOT_CXX_FLAGS_M ${ROOT_CXX_FLAGS})

	if (STAR_ROOT_CXX_FLAGS_M)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m${CMAKE_MATCH_3}")
		message(STATUS "StarCommon: Found -m${CMAKE_MATCH_3} option in $ROOT_CXX_FLAGS (root-config). Will add it to $CMAKE_CXX_FLAGS")

		if (CMAKE_MATCH_3 EQUAL 32)
			set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS FALSE)
		endif()

	endif()

else()
	message(FATAL_ERROR "StarCommon: FATAL: ROOT package not found")
endif()


message(STATUS "StarCommon: CMAKE_CXX_FLAGS = \"${CMAKE_CXX_FLAGS}\"")


add_definitions( -D__ROOT__ )

# Remove dependency of "install" target on "all" target. This allows to
# build and install individual libraries
set( CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE )


# Make use of the $STAR_HOST_SYS evironment variable. If it is set use it as the
# typical STAR installation prefix
set(STAR_ADDITIONAL_INSTALL_PREFIX ".")

if(DEFINED ENV{STAR_HOST_SYS})
	set(STAR_ADDITIONAL_INSTALL_PREFIX ".$ENV{STAR_HOST_SYS}")
endif()



#
# Builds a list of header files from which a ROOT dictionary can be created for
# a given subdirectory `star_lib_dir`. The list is put into the `headers_for_dict`
# variable that is returned to the parent scope. Only *.h and *.hh files
# are selected while any LinkDef files are ignored.
#
function( STAR_HEADERS_FOR_ROOT_DICTIONARY star_lib_dir headers_for_dict )

	# Get all header files in 'star_lib_dir'
	file(GLOB_RECURSE star_lib_dir_headers "${CMAKE_CURRENT_SOURCE_DIR}/${star_lib_dir}/*.h"
	                                       "${CMAKE_CURRENT_SOURCE_DIR}/${star_lib_dir}/*.hh")

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
# Generates a basic LinkDef header ${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_LinkDef.h
# by parsing the user provided header files with standard linux utilities awk and sed.
#
function(STAR_GENERATE_LINKDEF star_lib_dir dict_headers)

	# Set default name for LinkDef file
	set( linkdef_file "${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_LinkDef.h" )
	set_source_files_properties(${linkdef_file} PROPERTIES GENERATED TRUE)

	message(STATUS "StarCommon: Generating LinkDef header: ${linkdef_file}")

	# Check availability of required system tools
	find_program(EXEC_AWK NAMES gawk awk)
	find_program(EXEC_SED NAMES gsed sed)

	if(NOT EXEC_AWK OR NOT EXEC_SED)
		message(FATAL_ERROR "StarCommon: FATAL: STAR_GENERATE_LINKDEF function requires awk and sed commands")
	endif()

	cmake_parse_arguments(ARG "" "" "LINKDEF;LINKDEF_HEADERS" ${ARGN})

	# If provided by the user read the contents of their LinkDef file into a list
	set( linkdef_contents )

	if( ARG_LINKDEF )
		file(READ ${ARG_LINKDEF} linkdef_contents)
		# Convert file lines into a CMake list
		STRING(REGEX REPLACE ";" "\\\\;" linkdef_contents "${linkdef_contents}")
		STRING(REGEX REPLACE "\n" ";" linkdef_contents "${linkdef_contents}")
	endif()

	# Parse header files for ClassDef() statements and collect all entities into
	# a list
	set( dict_entities )
	set( dict_valid_headers )

	foreach( header ${ARG_LINKDEF_HEADERS} )

		# Parse for special namespace marked with $NMSPC in the header file
		set( my_exec_cmd ${EXEC_AWK} "match($0,\"namespace[[:space:]]+(\\\\w+).*\\\\$NMSPC\", a) { printf(a[1]\"\\r\") }" )
		execute_process(COMMAND ${my_exec_cmd} ${header} OUTPUT_VARIABLE extracted_namespace OUTPUT_STRIP_TRAILING_WHITESPACE)

		set( my_exec_cmd ${EXEC_AWK} "match($0,\"^[[:space:]]*ClassDef[[:space:]]*\\\\(([^#]+),.*\\\\)\",a){ printf(a[1]\"\\r\") }" )

		execute_process( COMMAND ${my_exec_cmd} ${header} COMMAND ${EXEC_SED} -e "s/\\s\\+/;/g"
			OUTPUT_VARIABLE extracted_dict_objects OUTPUT_STRIP_TRAILING_WHITESPACE )

		# Prepend dictionary types with namespace
		if( extracted_namespace )
			string(REGEX REPLACE "([^;]+)" "${extracted_namespace}::\\1" extracted_dict_objects "${extracted_dict_objects}")
		endif()

		list( APPEND dict_entities ${extracted_dict_objects} )

		if( extracted_dict_objects )
			list( APPEND dict_valid_headers ${header} )
		else()
			# if header base name matches linkdef_contents then use this header
			get_filename_component( header_base_name ${header} NAME_WE )

			if( "${linkdef_contents}" MATCHES "${header_base_name}" )
				list( APPEND dict_valid_headers ${header} )
			endif()
		endif()

	endforeach()


	# Special case dealing with StEvent containers
	set( dict_entities_stcontainers )

	if( "${star_lib_dir}" MATCHES "StEvent" )
		foreach( header ${ARG_LINKDEF_HEADERS} )
			set( my_exec_cmd ${EXEC_AWK} "match($0,\"^[[:space:]]*StCollectionDef[[:space:]]*\\\\(([^#]+)\\\\)\",a){ printf(a[1]\"\\r\") }" )
			
			execute_process( COMMAND ${my_exec_cmd} ${header} COMMAND ${EXEC_SED} -e "s/\\s\\+/;/g"
				OUTPUT_VARIABLE extracted_dict_objects OUTPUT_STRIP_TRAILING_WHITESPACE )

			list( APPEND dict_entities_stcontainers ${extracted_dict_objects} )

			if( extracted_dict_objects )
				list( APPEND dict_valid_headers ${header} )
			endif()
		endforeach()
	endif()

	# Assign new validated headers 
	set( ${dict_headers} ${dict_valid_headers} PARENT_SCOPE )

	# Write contents to the generated *_LinkDef.h file

	if( "${linkdef_contents}" MATCHES "pragma[ \t]+link[ \t]+off[ \t]+all" )
		file(WRITE ${linkdef_file} "#ifdef __CINT__\n\n")
	else()
		file(WRITE ${linkdef_file} "#ifdef __CINT__\n\n#pragma link off all globals;\n#pragma link off all classes;\n#pragma link off all functions;\n\n")
	endif()

	foreach( linkdef_line ${linkdef_contents} )
		if( "${linkdef_line}" MATCHES "#pragma[ \t]+link")
			file( APPEND ${linkdef_file} "${linkdef_line}\n" )
		endif()
	endforeach()
	
	file( APPEND ${linkdef_file} "\n\n// Collected dictionary entities\n\n" )

	foreach( dict_entity ${dict_entities} )
		if( NOT "${linkdef_contents}" MATCHES "class[ ]+${dict_entity}[< ;+-]+")
			file( APPEND ${linkdef_file} "#pragma link C++ class ${dict_entity}+;\n" )
		endif()
	endforeach()

	file( APPEND ${linkdef_file} "\n\n" )

	foreach( dict_entity ${dict_entities_stcontainers} )
		file( APPEND ${linkdef_file} "#pragma link C++ class StPtrVec${dict_entity}-;\n" )
		file( APPEND ${linkdef_file} "#pragma link C++ class StSPtrVec${dict_entity}-;\n" )
	endforeach()

	file( APPEND ${linkdef_file} "#endif\n" )

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
	set( dict_headers )
	star_generate_linkdef( ${star_lib_dir} dict_headers LINKDEF ${ARG_LINKDEF} LINKDEF_HEADERS ${linkdef_headers})

	foreach( header ${ARG_LINKDEF_HEADERS})
		get_filename_component( header_absolute_path ${header} ABSOLUTE )
		list( APPEND dict_headers "${header_absolute_path}" )
	endforeach()

	root_generate_dictionary( ${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_dict ${dict_headers}
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
	if( NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${star_lib_dir} )
		message( WARNING "StarCommon: Subdirectory \"${star_lib_dir}\" not found in ${CMAKE_CURRENT_SOURCE_DIR}. Skipping" )
		return()
	endif()

	get_filename_component( star_lib_name ${star_lib_dir} NAME )

	cmake_parse_arguments(ARG "" "" "LINKDEF;LINKDEF_HEADERS;LINKDEF_OPTIONS;EXCLUDE" "" ${ARGN})

	# Set default regex'es to exclude from globbed
	list( APPEND ARG_EXCLUDE "${star_lib_dir}.*macros;${star_lib_dir}.*doc;${star_lib_dir}.*examples" )

	# Deal with headers

	# Search for default LinkDef if not specified
	file( GLOB user_linkdefs "${CMAKE_CURRENT_SOURCE_DIR}/${star_lib_dir}/*LinkDef.h"
	                         "${CMAKE_CURRENT_SOURCE_DIR}/${star_lib_dir}/*LinkDef.hh" )

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
	file(GLOB_RECURSE sources "${CMAKE_CURRENT_SOURCE_DIR}/${star_lib_dir}/*.cxx"
	                          "${CMAKE_CURRENT_SOURCE_DIR}/${star_lib_dir}/*.cc"
	                          "${CMAKE_CURRENT_SOURCE_DIR}/${star_lib_dir}/*.cpp")

	if( ARG_EXCLUDE )
		FILTER_LIST( sources "${ARG_EXCLUDE}" )
	endif()

	add_library(${star_lib_name} SHARED ${sources} ${CMAKE_CURRENT_BINARY_DIR}/${star_lib_dir}_dict.cxx)

	get_subdirs( ${CMAKE_CURRENT_SOURCE_DIR}/${star_lib_dir} star_lib_subdirs )

	target_include_directories( ${star_lib_name} PRIVATE "${star_lib_subdirs}" )

	install( TARGETS ${star_lib_name}
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib"
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


# Creates a list of subdirectories starting with "St" of all directories passed
# to this function
function(STAR_INCLUDE_DIRECTORIES star_include_dirs)

	cmake_parse_arguments(ARG "" "" "" "" ${ARGN})

	set( inc_dirs "" )

	foreach( inc_dir ${ARG_UNPARSED_ARGUMENTS})

		list( APPEND inc_dirs "${CMAKE_SOURCE_DIR}/${inc_dir}" )

		# star-detectors is exceptional in a sense that it has too many subdirectories
		# So, don't include them in the final list. The user can add necessary
		# subdirectories by hand
		if( ${inc_dir} MATCHES "star-detectors")
			continue()
		endif()

		file( GLOB inc_subdirs ${inc_dir}/St* )

		# For each subdir do...
		foreach( inc_subdir ${inc_subdirs} )
			if( IS_DIRECTORY ${inc_subdir} )
				list( APPEND inc_dirs "${inc_subdir}" )
			endif()
		endforeach()

	endforeach()

	set( ${star_include_dirs} ${inc_dirs} PARENT_SCOPE)

endfunction()


# A list of regex'es to exclude from any processing in star_repo directories
set( STAR_BLACKLIST_DIR_NAMES
	.git
	cmake
	macros
	Mini
	Pool
	StarGenerator
	StBTofSimMaker
	StBTofMixerMaker
	StFgt
	StFtpc
	StHbt
	StJetMaker
	StTagsMaker
	StTofHitMaker
	StTrgDatFileReader
	StTrgMaker
	StTriggerUtilities
	Geometry
	StarAgmlChecker
	StarAgmlViewer
	StarGeometry
	StarVMCApplication
	Stv
	StVmcTools
	xgeometry
)

# special cases
set( St_base_LINKDEF_HEADERS "StRoot/St_base/Stypes.h" )
set( StEvent_LINKDEF_OPTIONS "-p;-D__STEVENT_CONTAINERS_CINT__" )
set( StMcEvent_LINKDEF_OPTIONS "-p;-D__STEVENT_CONTAINERS_CINT__" )


function( STAR_ADD_SUBDIRECTORY star_parent_dir )

	cmake_parse_arguments(ARG "" "" "INCLUDE_DIRS" "" ${ARGN})

	star_include_directories( include_dirs ${ARG_INCLUDE_DIRS} ${star_parent_dir} )


	file( GLOB star_lib_dir_candidates RELATIVE ${CMAKE_SOURCE_DIR}/${star_parent_dir} ${star_parent_dir}/* )
	#file(GLOB star_lib_dir_candidates ${star_parent_dir}/*)

	FILTER_LIST( star_lib_dir_candidates "${STAR_BLACKLIST_DIR_NAMES}" )

	# Save include dirs property for the current dir. This is needed because we
	# don't go into the directories
	get_directory_property(parent_include_dirs INCLUDE_DIRECTORIES)

	set( star_lib_dirs "" )

	foreach( star_lib_dir ${star_lib_dir_candidates} )
		if( NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${star_parent_dir}/${star_lib_dir} )
			continue()
		endif()

		# The following setup is necessary to fool the call to
		# star_add_library() and other functions because we don't really switch
		# the directories but traverse them anyway
		set( CMAKE_CURRENT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/${star_parent_dir}" )
		set( CMAKE_CURRENT_BINARY_DIR "${CMAKE_BINARY_DIR}/${star_parent_dir}" )
		set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${star_parent_dir}" )

		set_directory_properties( PROPERTIES INCLUDE_DIRECTORIES "${parent_include_dirs};${include_dirs}" )

		list( APPEND star_lib_dirs "${star_lib_dir}" )

		star_add_library( ${star_lib_dir} LINKDEF_HEADERS "${${star_lib_dir}_LINKDEF_HEADERS}" LINKDEF_OPTIONS "${${star_lib_dir}_LINKDEF_OPTIONS}" )

		if( ${star_lib_dir} MATCHES "StBFChain" )
			install(DIRECTORY "${star_parent_dir}/StBFChain" DESTINATION "${CMAKE_BINARY_DIR}/StRoot")
		endif()

	endforeach()

	set_directory_properties( PROPERTIES INCLUDE_DIRECTORIES "${parent_include_dirs}" )

	# A collective target to build all libraries in this project. Can be used to
	# build all specified targets from a parent project
	add_custom_target( ${star_parent_dir} DEPENDS "${star_lib_dirs}" )

endfunction()


#
# Flattens the hierarchy of header files found in `parent_dir` at 1 level deep
# by copying them to `${STAR_ADDITIONAL_INSTALL_PREFIX}/include/`
#
function( STAR_PREINSTALL_HEADERS parent_dir )

	# Get all header files in 'parent_dir'
	file( GLOB header_files "${CMAKE_CURRENT_SOURCE_DIR}/${parent_dir}/*/*.h"
	                        "${CMAKE_CURRENT_SOURCE_DIR}/${parent_dir}/*/*.hh" )

	foreach( header_file ${header_files})
		get_filename_component( header_file_name ${header_file} NAME )
		configure_file( "${header_file}" "${STAR_ADDITIONAL_INSTALL_PREFIX}/include/${header_file_name}" COPYONLY )
	endforeach()

endfunction()
