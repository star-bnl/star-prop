# Load this cmake file only once
if( StarCommonLoaded )
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
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
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


# Since most of STAR projects depend on ROOT check the flags and use the same
if(ROOT_FOUND)

	string(REGEX MATCH "(^|[\t ]+)-m([\t ]*)(32|64)([\t ]+|$)" STAR_ROOT_CXX_FLAGS_M ${ROOT_CXX_FLAGS})

	if (STAR_ROOT_CXX_FLAGS_M)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m${CMAKE_MATCH_3}")
		message(STATUS "StarCommon: Found -m${CMAKE_MATCH_3} option in $ROOT_CXX_FLAGS (root-config). Will add it to $CMAKE_CXX_FLAGS")

		if (CMAKE_MATCH_3 EQUAL 32)
			set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
		endif()

	endif()

else()
	message(FATAL_ERROR "FATAL: ROOT package not found")
endif()


message(STATUS "StarCommon: CMAKE_CXX_FLAGS = \"${CMAKE_CXX_FLAGS}\"")


#
# Builds a list of header files from which a ROOT dictionary can be created for
# a given subdirectory `stroot_dir`. The list is put into the `headers_for_dict`
# variable that is returned to the parent scope. Only *.h and *.hh files
# containing ROOT's ClassDef macro are selected while any LinkDef files are
# ignored.
#
function(STAR_HEADERS_FOR_ROOT_DICTIONARY stroot_dir headers_for_dict)

	find_program(EXEC_GREP NAMES grep)

	if(NOT EXEC_GREP)
		message(FATAL_ERROR "FATAL: STAR_HEADERS_FOR_ROOT_DICTIONARY function requires grep")
	endif()

	# Get all header files in 'stroot_dir'
	file(GLOB_RECURSE stroot_dir_headers "${stroot_dir}/*.h" "${stroot_dir}/*.hh")

	# Create an empty list
	set(valid_headers)

	foreach(header ${stroot_dir_headers})

		# Skip LinkDef files from globbing result
		if(header MATCHES LinkDef)
			message(STATUS "WARNING: Skipping LinkDef header ${header}")
			continue()
		endif()

		# Check for at least one ClassDef macro in the header file
		execute_process(COMMAND ${EXEC_GREP} -m1 -H ClassDef ${header} RESULT_VARIABLE exit_code OUTPUT_QUIET)

		if (NOT ${exit_code})
			# May want to verify that the header file does exist in the include directories
			#get_filename_component( headerFileName ${userHeader} NAME)
			#find_file(headerFile ${headerFileName} HINTS ${incdirs})

			list(APPEND valid_headers ${header})
		else()
			message(STATUS "WARNING: No ClassDef macro found in ${header}")
		endif()

	endforeach()

	set(${headers_for_dict} ${valid_headers} PARENT_SCOPE)

endfunction()


#
# Generates a basic LinkDef header ${CMAKE_CURRENT_BINARY_DIR}/${stroot_dir}_LinkDef.h
# by parsing the user provided header files with standard linux utilities awk and sed.
#
function(STAR_GENERATE_LINKDEF stroot_dir)

	# Set default name for LinkDef file
	set( linkdef_file "${CMAKE_CURRENT_BINARY_DIR}/${stroot_dir}_LinkDef.h" )
	set_source_files_properties(${linkdef_file} PROPERTIES GENERATED TRUE)

	message(STATUS "Generating LinkDef header: ${linkdef_file}")

	# Check availability of required system tools
	find_program(EXEC_AWK NAMES gawk awk)
	find_program(EXEC_SED NAMES gsed sed)

	if(NOT EXEC_AWK OR NOT EXEC_SED)
		message(FATAL_ERROR "StarCommon: FATAL: STAR_GENERATE_LINKDEF function requires awk and sed commands")
	endif()

	CMAKE_PARSE_ARGUMENTS(ARG "" "" "LINKDEF;LINKDEF_HEADERS" ${ARGN})

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

	foreach( header ${ARG_LINKDEF_HEADERS} )
		set( my_exec_cmd ${EXEC_AWK} "match($0,\"^[[:space:]]*ClassDef(.*)\\\\(([^#]+),(.*)\\\\)\",a){ printf(a[2]\"\\r\") }" )

		execute_process( COMMAND ${my_exec_cmd} ${header} COMMAND ${EXEC_SED} -e "s/\\s\\+/;/g"
			RESULT_VARIABLE exit_code OUTPUT_VARIABLE extracted_dict_objects ERROR_VARIABLE extracted_dict_objects
			OUTPUT_STRIP_TRAILING_WHITESPACE )
		list( APPEND dict_entities ${extracted_dict_objects} )
	endforeach()


	# Special case dealing with StEvent containers
	set( dict_entities_stcontainers )

	if( "${stroot_dir}" MATCHES "StEvent" )
		foreach( header ${ARG_LINKDEF_HEADERS} )
			set( my_exec_cmd ${EXEC_AWK} "match($0,\"^[[:space:]]*StCollectionDef(.*)\\\\(([^#]+)\\\\)\",a){ printf(a[2]\"\\r\") }" )
			
			execute_process( COMMAND ${my_exec_cmd} ${header} COMMAND ${EXEC_SED} -e "s/\\s\\+/;/g"
				RESULT_VARIABLE exit_code OUTPUT_VARIABLE extracted_dict_objects ERROR_VARIABLE extracted_dict_objects
				OUTPUT_STRIP_TRAILING_WHITESPACE )
			list( APPEND dict_entities_stcontainers ${extracted_dict_objects} )
		endforeach()
	endif()

	
	# Write contents to the generated *_LinkDef.h file
	file(WRITE ${linkdef_file}
		"#ifdef __CINT__\n\n#pragma link off all globals;\n#pragma link off all classes;\n#pragma link off all functions;\n\n")

	foreach( linkdef_line ${linkdef_contents} )
		if( "${linkdef_line}" MATCHES "#pragma")
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

	file( APPEND ${linkdef_file} "\n#endif\n" )

endfunction()


#
# Generates a ROOT dictionary for `stroot_dir`.
#
function(STAR_GENERATE_DICTIONARY stroot_dir)

	CMAKE_PARSE_ARGUMENTS(ARG "" "" "LINKDEF;LINKDEF_HEADERS;LINKDEF_OPTIONS" "" ${ARGN})

	# Generate a basic LinkDef file and, if available, merge with the one
	# provided by the user
	STAR_GENERATE_LINKDEF(${stroot_dir} LINKDEF ${ARG_LINKDEF} LINKDEF_HEADERS ${linkdef_headers})

	# Set default options
	set( linkdef_options "-p" )

	if( ARG_LINKDEF_OPTIONS )
		set(linkdef_options ${ARG_LINKDEF_OPTIONS})
	endif()

	root_generate_dictionary(${stroot_dir}_dict ${linkdef_headers} LINKDEF ${CMAKE_CURRENT_BINARY_DIR}/${stroot_dir}_LinkDef.h OPTIONS ${linkdef_options})

endfunction()


#
# Adds a target to build a library from all source files (*.cxx, *.cc, and *.cpp)
# recursively found in the specified subdirectory `stroot_dir`. It is possible
# to EXCLUDE some files matching an optional pattern.
#
function(STAR_ADD_LIBRARY stroot_dir)

	CMAKE_PARSE_ARGUMENTS(ARG "" "" "LINKDEF;LINKDEF_HEADERS;LINKDEF_OPTIONS;EXCLUDE" "" ${ARGN})

	# Deal with headers
	if( NOT TARGET ${stroot_dir}_dict.cxx )

		# If the user provided header files use them. Otherwise collect headers
		# for the dictionary automatically
		set( linkdef_headers )

		if( ARG_LINKDEF_HEADERS )
			file( GLOB_RECURSE linkdef_headers ${ARG_LINKDEF_HEADERS} )
		else()
			STAR_HEADERS_FOR_ROOT_DICTIONARY( ${stroot_dir} linkdef_headers )
		endif()

		if( ARG_EXCLUDE )
			# Starting cmake 3.6 one can simply use list( FILTER ... )
			#list( FILTER sources EXCLUDE REGEX "${ARG_EXCLUDE}" )
			foreach(source_file ${linkdef_headers})
				if("${source_file}" MATCHES ${ARG_EXCLUDE})
					# Remove current source_file from the list
					list(REMOVE_ITEM linkdef_headers ${source_file})
				endif()
			endforeach()
		endif()

		star_generate_dictionary( ${stroot_dir}
			LINKDEF ${ARG_LINKDEF} LINKDEF_HEADERS ${ARG_LINKDEF_HEADERS} LINKDEF_OPTIONS ${ARG_LINKDEF_OPTIONS}
		)

	endif()

	# Deal with sources
	file(GLOB_RECURSE sources "${stroot_dir}/*.cxx" "${stroot_dir}/*.cc" "${stroot_dir}/*.cpp")

	if( ARG_EXCLUDE )
		# Starting cmake 3.6 one can simply use list( FILTER ... )
		#list( FILTER sources EXCLUDE REGEX "${ARG_EXCLUDE}" )
		foreach(source_file ${sources})
			if("${source_file}" MATCHES ${ARG_EXCLUDE})
				# Remove current source_file from the list
				list(REMOVE_ITEM sources ${source_file})
			endif()
		endforeach()
	endif()

	add_library(${stroot_dir} SHARED ${sources} ${stroot_dir}_dict.cxx)

endfunction()


function( FILTER_LIST arg_list arg_regexs )

	# Starting cmake 3.6 one can simply use list( FILTER ... )
	#list( FILTER sources EXCLUDE REGEX "${ARG_EXCLUDE}" )

	foreach( item ${${arg_list}} )
		foreach( regex ${${arg_regexs}} )

			if( ${item} MATCHES "${regex}" )
				list(REMOVE_ITEM ${arg_list} ${item})
				break()
			endif()

		endforeach()
	endforeach()

	set( ${arg_list} ${${arg_list}} PARENT_SCOPE)

endfunction()


# Make use of the $STAR_HOST_SYS evironment variable. If it is set use it as the
# typical STAR installation prefix
set(STAR_ADDITIONAL_INSTALL_PREFIX ".")

if(DEFINED ENV{STAR_HOST_SYS})
	set(STAR_ADDITIONAL_INSTALL_PREFIX ".$ENV{STAR_HOST_SYS}")
endif()
