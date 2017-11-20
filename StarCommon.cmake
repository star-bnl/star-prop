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
# Generates a ROOT dictionary for `stroot_dir`.
#
function(STAR_GENERATE_DICTIONARY stroot_dir)

	CMAKE_PARSE_ARGUMENTS(ARG "" "" "LINKDEF;LINKDEF_HEADERS;LINKDEF_OPTIONS" "" ${ARGN})

	# If the user provided header files use them. Otherwise collect headers
	# for the dictionary automatically
	set( linkdef_headers )

	if( ARG_LINKDEF_HEADERS )
		set( linkdef_headers ${ARG_LINKDEF_HEADERS} )
	else()
		STAR_HEADERS_FOR_ROOT_DICTIONARY( ${stroot_dir} linkdef_headers )
	endif()

	# Set default name for LinkDef file
	set( linkdef_file "${CMAKE_CURRENT_BINARY_DIR}/${stroot_dir}_LinkDef.h" )

	# Generate a basic LinkDef file if not provided by the user
	if( ARG_LINKDEF )
		set( linkdef_file ${ARG_LINKDEF} )
	else()
		root_generate_linkdef(${linkdef_file} HEADERS ${linkdef_headers})
	endif()

	# Set default options
	set( linkdef_options "-p" )

	if( ARG_LINKDEF_OPTIONS )
		set(linkdef_options ${ARG_LINKDEF_OPTIONS})
	endif()

	root_generate_dictionary(${stroot_dir}_dict ${linkdef_headers} LINKDEF ${linkdef_file} OPTIONS ${linkdef_options})

endfunction()


# Make use of the $STAR_HOST_SYS evironment variable. If it is set use it as the
# typical STAR installation prefix
set(STAR_ADDITIONAL_INSTALL_PREFIX ".")

if(DEFINED ENV{STAR_HOST_SYS})
	set(STAR_ADDITIONAL_INSTALL_PREFIX ".$ENV{STAR_HOST_SYS}")
endif()
