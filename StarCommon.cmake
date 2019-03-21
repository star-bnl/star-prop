# Load this cmake file only once
if(StarCommonLoaded)
	message(STATUS "StarCommon: Should be included only once")
	return()
else()
	set(StarCommonLoaded TRUE)
endif()

# By default build shared libraries but allow the user to change if desired
OPTION( BUILD_SHARED_LIBS "Global flag to cause add_library to create shared libraries if on" ON )

# Special treatment for gcc linker options to get a linux-like behavior on MacOS X
if(APPLE)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -undefined dynamic_lookup")
endif()


find_program(ROOT_DICTGEN_EXECUTABLE NAMES rootcling rootcint HINTS $ENV{ROOTSYS}/bin)


# Define common STAR_ and CMAKE_ variables used to build the STAR code

# -D__ROOT__ is used by classes in StarClassLibrary guarding calls to ClassDef() macro
# -D_UCMLOGGER_ is used in StStarLogger
# -DNEW_DAQ_READER is used in StTofHitMaker
# -Df2cFortran required by starsim and cern/pro/include/cfortran/cfortran.h
set(STAR_C_CXX_DEFINITIONS "-D__ROOT__ -D_UCMLOGGER_ -DNEW_DAQ_READER -Df2cFortran")
# CPP_DATE, CPP_TIME, CPP_TITLE_CH, and CPP_VERS are used by gcalor and by asps/Simulation/starsim/aversion.F
# CERNLIB_CG is used by asps/Simulation/geant321/gdraw/gdcota.F
# CERNLIB_COMIS is used by asps/Simulation/geant321/gxint/gxcs.F
# GFORTRAN is used by asps/Simulation/starsim/atmain/etime.F
set(STAR_Fortran_DEFINITIONS "-DCERNLIB_TYPE -DCERNLIB_DOUBLE -DCERNLIB_NOQUAD -DCERNLIB_LINUX \
-DCPP_DATE=0 -DCPP_TIME=0 -DCPP_TITLE_CH=\"'dummy'\" -DCPP_VERS=\"'dummy'\" -DCERNLIB_CG -DCERNLIB_COMIS -DGFORTRAN")
set(STAR_Fortran_FLAGS "-fd-lines-as-code -std=legacy -fno-second-underscore -fno-automatic")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STAR_C_CXX_DEFINITIONS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STAR_C_CXX_DEFINITIONS}")
set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} ${STAR_Fortran_FLAGS} ${STAR_Fortran_DEFINITIONS}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

# Remove dependency of "install" target on "all" target. This allows to
# build and install individual libraries
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

# Make use of the $STAR_HOST_SYS evironment variable. If it is set use it as the
# typical STAR installation prefix
set(STAR_ADDITIONAL_INSTALL_PREFIX ".")
if(DEFINED ENV{STAR_HOST_SYS})
	set(STAR_ADDITIONAL_INSTALL_PREFIX ".$ENV{STAR_HOST_SYS}")
endif()

set(STAR_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(STAR_LIB_DIR_BLACKLIST
	StarVMC/geant3            # how is it used?
	StarVMC/GeoTestMaker      # blacklisted in cons
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
	StRoot/StAngleCorrMaker   # blacklisted in cons
	StRoot/StarGenerator
	StRoot/StDaqClfMaker      # blacklisted in cons
	StRoot/StEbye2ptMaker     # blacklisted in cons
	StRoot/StEbyePool         # blacklisted in cons
	StRoot/StEbyeScaTagsMaker # blacklisted in cons
	StRoot/StEEmcPool         # requires subdir processing
	StRoot/StFgtPool          # blacklisted in cons
	StRoot/StFlowMaker        # missing from lib/
	StRoot/StFtpcV0Maker      # blacklisted in cons
	StRoot/St_geom_Maker      # requires qt4/include/QtGui
	StRoot/StHbtMaker         # fortran error
	StRoot/StHighptPool       # blacklisted in cons
	StRoot/StJetFinder        # needs FindFastJet.cmake
	StRoot/StJetMaker
	StRoot/StShadowMaker      # blacklisted in cons, crypted code
	StRoot/StSpinMaker        # blacklisted in cons, error in fortran code
	StRoot/StSpinPool         # blacklisted in cons
	StRoot/StStrangePool      # blacklisted in cons
	StRoot/StTofPool          # missing from lib/
)


#
# Preselects all header files found in a `star_lib_dir` directory to be used by
# rootcint/rootcling during ROOT dictionary creation. The list is put into the
# `headers_for_dict` variable which is returned to the parent scope. Only files
# with extensions *.h, *.hh, and *.hpp are considered while files containing
# a 'LinkDef' substring are ignored.
#
function(STAR_HEADERS_FOR_ROOT_DICTIONARY star_lib_dir headers_for_dict)
	# Get all header files in 'star_lib_dir'
	file(GLOB_RECURSE star_lib_dir_headers "${star_lib_dir}/*.h"
	                                       "${star_lib_dir}/*.hh"
	                                       "${star_lib_dir}/*.hpp")
	# Create an empty list
	set(valid_headers)

	# star_lib_dir_headers should containd absolute paths to globed headers
	foreach(full_path_header ${star_lib_dir_headers})

		get_filename_component(header_file_name ${full_path_header} NAME)
		string(TOLOWER ${header_file_name} header_file_name)
		# Skip LinkDef files from globbing result
		if(${header_file_name} MATCHES "linkdef")
			continue()
		endif()

		list(APPEND valid_headers ${full_path_header})
	endforeach()

	set(${headers_for_dict} ${valid_headers} PARENT_SCOPE)
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
		COMMAND "${STAR_CMAKE_DIR}/gen_linkdef.sh" ${gen_linkdef_args}
		DEPENDS "${STAR_CMAKE_DIR}/gen_linkdef.sh" ${ARG_LINKDEF_HEADERS}
		VERBATIM)
endfunction()


#
# Generates a ROOT dictionary for `star_lib_dir` in ${STAR_SRC}.
#
function(STAR_GENERATE_DICTIONARY star_lib_name star_lib_dir star_lib_dir_out)

	cmake_parse_arguments(ARG "" "" "LINKDEF_HEADERS;LINKDEF_OPTIONS;EXCLUDE" "" ${ARGN})

	# Search for default LinkDef if not specified
	file( GLOB user_linkdefs "${star_lib_dir}/*LinkDef.h"
	                         "${star_lib_dir}/*LinkDef.hh" )

	# Get the first LinkDef from the list
	if( user_linkdefs )
		list( GET user_linkdefs 0 user_linkdef )
	endif()

	# Preselect header files from `star_lib_dir`
	star_headers_for_root_dictionary(${star_lib_dir} linkdef_headers)

	FILTER_LIST(linkdef_headers EXCLUDE ${ARG_EXCLUDE})

	# This is a hack for the call to this function from STAR_ADD_LIBRARY_GEOMETRY() where the
	# headers are generated at runtime and cannot be globbed when cmake is invoked. So, we need
	# to pass the necessary headers in the LINKDEF_HEADERS argument.
	if(NOT linkdef_headers)
		set(linkdef_headers ${ARG_LINKDEF_HEADERS})
	endif()

	# Generate a basic LinkDef file and, if available, merge with the one
	# provided by the user
	star_generate_linkdef( ${star_lib_dir_out} LINKDEF ${user_linkdef} LINKDEF_HEADERS ${linkdef_headers} )

	# Prepare include directories to be used during ROOT dictionary generation.
	# These directories are tied to the `star_lib_name` target via the
	# INCLUDE_DIRECTORIES property.
	get_target_property( target_include_dirs ${star_lib_name} INCLUDE_DIRECTORIES )
	string( REGEX REPLACE "([^;]+)" "-I\\1" dict_include_dirs "${target_include_dirs}" )

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

	# Get first optional unnamed parameter
	set(user_lib_name ${ARGV1})
	set(star_lib_name_for_tables)
	if(user_lib_name)
		set(star_lib_name_for_tables ${star_lib_name})
		set(star_lib_name ${user_lib_name})
	endif()

	# Deal with sources

	GET_EXCLUDE_LIST( ${star_lib_name} star_lib_exclude )

	file(GLOB_RECURSE sources_cpp
		"${star_lib_dir_abs}/*.cxx"
		"${star_lib_dir_abs}/*.cc"
		"${star_lib_dir_abs}/*.c"
		"${star_lib_dir_abs}/*.cpp")
	FILTER_LIST(sources_cpp EXCLUDE ${star_lib_exclude})

	file(GLOB_RECURSE f_files "${star_lib_dir_abs}/*.F")
	FILTER_LIST(f_files EXCLUDE ${star_lib_exclude})
	star_process_f(${star_lib_name} "${f_files}" ${star_lib_dir_out} sources_F)

	file(GLOB_RECURSE g_files "${star_lib_dir_abs}/*.g")
	FILTER_LIST(g_files EXCLUDE ${star_lib_exclude})
	star_process_g("${g_files}" ${star_lib_dir_out} sources_gtoF)

	file(GLOB_RECURSE idl_files "${star_lib_dir_abs}/*.idl")
	FILTER_LIST(idl_files EXCLUDE ${star_lib_exclude})
	string(REPLACE "+" "\\\\+" idl_path_exclude_regex "${star_lib_dir_abs}/idl")
	FILTER_LIST(idl_files EXCLUDE ${idl_path_exclude_regex})
	star_process_idl("${idl_files}" "${star_lib_name_for_tables}" ${star_lib_dir_out} sources_idl headers_idl)

	set(sources ${sources_cpp} ${sources_idl} ${sources_gtoF} ${sources_F})

	# XXX The hardcoded .cxx extension below should be defined by cmake?
	add_library(${star_lib_name} ${sources} ${star_lib_dir_out}_dict.cxx)

	# Output the library to the respecitve subdirectory in the binary directory
	set_target_properties(${star_lib_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${star_lib_dir_out})

	GET_SUBDIRS(${star_lib_dir_abs} star_lib_subdirs INCLUDE_PARENT)
	target_include_directories(${star_lib_name} PRIVATE
		"${star_lib_subdirs}"
		"${CMAKE_CURRENT_BINARY_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/include/tables/${star_lib_name_for_tables}")

	# Generate the _dict.cxx file for the library
	star_generate_dictionary(${star_lib_name} ${star_lib_dir_abs} ${star_lib_dir_out}
		LINKDEF_HEADERS ${${star_lib_name}_LINKDEF_HEADERS}
		LINKDEF_OPTIONS "-p;-D__ROOT__"
		EXCLUDE ${star_lib_exclude})

	install(TARGETS ${star_lib_name}
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL
		ARCHIVE DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL)

	install(FILES ${headers_idl}
		DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/include/tables/${star_lib_name_for_tables}" OPTIONAL)

endfunction()



macro(FILTER_LIST arg_list)

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
macro(GET_SUBDIRS directories subdirectories)

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
# From the path provided by the user (`star_lib_dir`) extracts the library
# target name (`lib_name`), an absolute path to the library source code
# (`path_abs`), and the corresponding output path where the generated or built
# code can be placed (`path_out`). The input path `star_lib_dir` can be either
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


#
# Flattens the hierarchy of header files found in select subdirectories in
# `${STAR_SRC}` by copying them to `destination_dir` at the same level
#
function(STAR_PREINSTALL_HEADERS destination_dir)

	# Collect files from some subdirectiries
	file(GLOB header_files
		"${STAR_SRC}/StRoot/*/*.h"
		"${STAR_SRC}/StRoot/*/*.hh"
		"${STAR_SRC}/StRoot/*/*.inc"
		"${STAR_SRC}/StarVMC/*/*.h"
		"${STAR_SRC}/StarVMC/*/*.hh"
		"${STAR_SRC}/StarVMC/*/*.inc"
		"${STAR_SRC}/asps/rexe/TGeant3/*.h"
		"${STAR_SRC}/pams/*/inc/*.h"
		"${STAR_SRC}/pams/*/inc/*.inc")

	foreach( header_file ${header_files} )
		get_filename_component( header_file_name ${header_file} NAME )
		configure_file( "${header_file}" "${destination_dir}/${header_file_name}" COPYONLY )
	endforeach()

endfunction()


#
# Creates ROOT TGeo geometries from xml files
#
function(STAR_ADD_LIBRARY_GEOMETRY star_lib_dir)

	star_target_paths(${star_lib_dir} star_lib_name star_lib_dir_abs star_lib_dir_out)

	# Change the name of the library/target and output directory if an
	# optional unnamed parameter is provided by the user
	set(user_lib_name ${ARGV1})
	if(user_lib_name)
		string(REPLACE ${star_lib_name} ${user_lib_name} star_lib_dir_out ${star_lib_dir_out})
		set(star_lib_name ${user_lib_name})
	endif()

	set(geo_py_parser ${STAR_SRC}/mgr/agmlParser.py)

	find_program(EXEC_PYTHON NAMES python2.7 python2)

	# Exclude some xml files
	file(GLOB_RECURSE geo_xml_files "${star_lib_dir_abs}/*.xml")
	FILTER_LIST( geo_xml_files EXCLUDE "Compat" )

	# The geometry library can be generated by choosing either of the two
	# available options "RootTGeo" and "GeantGeo". Default is "RootTGeo"
	if(${ARGV2} STREQUAL "GeantGeo")
		star_parse_geoxml_GeantGeo("${geo_xml_files}" ${star_lib_dir_out} geo_sources)
	else()
		star_parse_geoxml_RootTGeo("${geo_xml_files}" ${star_lib_dir_out} geo_sources geo_headers)
		list(APPEND geo_sources ${star_lib_dir_out}_dict.cxx)
	endif()

	add_library(${star_lib_name} ${geo_sources})
	set_target_properties(${star_lib_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${star_lib_dir_out})

	if(${ARGV2} STREQUAL "GeantGeo")
		# The geometry library is built from ${STAR_SRC}/pams/geometry. Not to be confused with
		# the geometry_Tables build from *.idl files in the same directory
		target_link_libraries(${star_lib_name} geometry)
	else()
		# Generate the _dict.cxx file for the library
		star_generate_dictionary(${star_lib_name} ${star_lib_dir_out} ${star_lib_dir_out}
			LINKDEF_HEADERS ${geo_headers} #${geo_headers_orig}
			LINKDEF_OPTIONS "-p;-D__ROOT__")
		target_include_directories(${star_lib_name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
	endif()

	# Get relative path for the generated headers to be used at installation
	# stage
	file(RELATIVE_PATH geo_headers_rel_path ${CMAKE_CURRENT_BINARY_DIR} ${star_lib_dir_out})
	install(TARGETS ${star_lib_name}
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL
		ARCHIVE DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL
		PUBLIC_HEADER DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/include/${geo_headers_rel_path}" OPTIONAL)
endfunction()


function(STAR_PARSE_GEOXML_GeantGeo geo_xml_files out_dir out_sources)
	# For each .xml file found in `star_lib_dir` (e.g. $STAR_SRC/StarVMC/Geometry) generate the
	# source and header files
	foreach(geo_xml_file ${geo_xml_files})
		get_filename_component(geo_name ${geo_xml_file} NAME_WE)
		set(geo_agesrc ${out_dir}/${geo_name}.age)

		add_custom_command(
			OUTPUT ${geo_agesrc}
			COMMAND ${EXEC_PYTHON} ${geo_py_parser} --file=${geo_xml_file} --module=${geo_name} --export=Mortran > ${geo_agesrc}
			DEPENDS ${geo_py_parser} ${geo_xml_file})

		list(APPEND geo_sources_generated ${geo_agesrc})
	endforeach()

	star_process_g("${geo_sources_generated};${STAR_SRC}/StarVMC/xgeometry/xgeometry.age" ${out_dir} out_sources_)

	# Return generated list
	set(${out_sources} ${out_sources_} PARENT_SCOPE)
endfunction()


function(STAR_PARSE_GEOXML_RootTGeo geo_xml_files out_dir out_sources out_headers)
	# For each .xml file found in `star_lib_dir` (e.g. $STAR_SRC/StarVMC/Geometry) generate the
	# source and header files
	foreach(geo_xml_file ${geo_xml_files})
		get_filename_component(geo_name ${geo_xml_file} NAME_WE)
		set(geo_header ${out_dir}/${geo_name}.h)
		set(geo_source ${out_dir}/${geo_name}.cxx)

		add_custom_command(
			OUTPUT ${geo_source} ${geo_header}
			COMMAND ${EXEC_PYTHON} ${geo_py_parser} --file=${geo_xml_file} --module=${geo_name} --export=AgROOT --path=${out_dir}
			DEPENDS ${geo_py_parser} ${geo_xml_file})

		if(${geo_name} MATCHES "Config")
			list(APPEND geo_config_headers ${geo_header})
		endif()
		list(APPEND geo_sources_generated ${geo_source})
		list(APPEND geo_headers_generated ${geo_header})
	endforeach()

	# Create a string by replacing ; (i.e. semicolon) with gcc compiler options
	string(REGEX REPLACE "([^;]+);" "-include \\1 " geo_config_headers_include "${geo_config_headers};")
	# Special treatment required for the aggregate geometry file because the generated files do
	# not include proper header files! Including that many files is likely to slow down the
	# compilation
	set_source_files_properties(${out_dir}/StarGeo.cxx
		PROPERTIES COMPILE_FLAGS "${geo_config_headers_include}")

	# Return generated lists
	set(${out_sources} ${geo_sources_generated} PARENT_SCOPE)
	set(${out_headers} ${geo_headers_generated} PARENT_SCOPE)
endfunction()


function(STAR_ADD_LIBRARY_STARSIM starsim_dir)

	star_target_paths(${starsim_dir} dummy starsim_dir_abs star_lib_dir_out)

	file(GLOB_RECURSE cxx_files "${starsim_dir_abs}/*.cxx")
	file(GLOB_RECURSE c_files "${starsim_dir_abs}/*.c")
	file(GLOB_RECURSE f_files "${starsim_dir_abs}/*.F")
	# Also find all *.g and *.age files. They need to be pre-processed with agetof
	file(GLOB_RECURSE g_files "${starsim_dir_abs}/*.g")
	star_process_g("${g_files}" ${star_lib_dir_out} f_g_files)
	file(GLOB_RECURSE age_files "${starsim_dir_abs}/*.age")
	star_process_g("${age_files}" ${star_lib_dir_out} f_age_files)

	set(starsim_sources ${cxx_files} ${c_files} ${f_files} ${f_g_files} ${f_age_files})
	set(starsimlib_EXCLUDE
		"${starsim_dir_abs}/acmain.cxx"
		"${starsim_dir_abs}/dzdoc/dzddiv.F"
		"${starsim_dir_abs}/atlroot/atlrootDict.cxx"
		"${starsim_dir_abs}/deccc/cschar.c")
	FILTER_LIST(starsim_sources EXCLUDE ${starsimlib_EXCLUDE})

	set(starsim_dict ${star_lib_dir_out}/atlroot/atlroot_DictInc.cxx)
	add_custom_command(
		OUTPUT ${starsim_dict}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${star_lib_dir_out}/atlroot
		COMMAND ${ROOT_DICTGEN_EXECUTABLE} -cint -f  ${starsim_dict} -c -p -D__ROOT__
		-I${starsim_dir_abs}/atlroot/ agconvert.h aroot.h ${starsim_dir_abs}/atlroot/LinkDef.h
		DEPENDS ${starsim_dir_abs}/atlroot/agconvert.h
		        ${starsim_dir_abs}/atlroot/aroot.h
		        ${starsim_dir_abs}/atlroot/LinkDef.h
		VERBATIM )

	set(starsim_INCLUDE_DIRECTORIES
		"${starsim_dir_abs}/include"
		"${STAR_SRC}/asps/Simulation/geant321/include"
		"${CERNLIB_INCLUDE_DIR}")

	add_library(starsimlib STATIC ${starsim_sources} ${starsim_dict})
	GET_SUBDIRS(${starsim_dir_abs} starsim_subdirs INCLUDE_PARENT)
	target_include_directories(starsimlib PRIVATE ${starsim_subdirs} ${starsim_INCLUDE_DIRECTORIES})
	set_target_properties(starsimlib PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${star_lib_dir_out})

	install(TARGETS starsimlib
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL
		ARCHIVE DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL)

endfunction()


#
# Creates a library from fortran sources without doing any preprocessing
#
function(STAR_ADD_LIBRARY_BASIC star_lib_dir)

	star_target_paths(${star_lib_dir} star_lib_name star_lib_dir_abs star_lib_dir_out)

	# Change the name of the library/target and output directory if an
	# optional unnamed parameter is provided by the user
	set(user_lib_name ${ARGV1})
	if(user_lib_name)
		string(REPLACE ${star_lib_name} ${user_lib_name} star_lib_dir_out ${star_lib_dir_out})
		set(star_lib_name ${user_lib_name})
	endif()

	file(GLOB_RECURSE cxx_files "${star_lib_dir_abs}/*.cxx")
	FILTER_LIST(cxx_files EXCLUDE ${${star_lib_name}_EXCLUDE})
	file(GLOB_RECURSE c_files "${star_lib_dir_abs}/*.c")
	FILTER_LIST( c_files EXCLUDE ${${star_lib_name}_EXCLUDE} )
	file(GLOB_RECURSE f_files "${star_lib_dir_abs}/*.F")
	FILTER_LIST( f_files EXCLUDE ${${star_lib_name}_EXCLUDE} )
	# Also find all *.g and *.age files. They need to be processed with agetof
	file(GLOB_RECURSE g_files "${star_lib_dir_abs}/*.g")
	star_process_g("${g_files}" ${star_lib_dir_out} f_g_files)
	file(GLOB_RECURSE age_files "${star_lib_dir_abs}/*.age")
	star_process_g("${age_files}" ${star_lib_dir_out} f_age_files)

	add_library(${star_lib_name} STATIC ${f_files} ${f_g_files} ${f_age_files} ${cxx_files} ${c_files})
	GET_SUBDIRS(${star_lib_dir_abs} star_lib_subdirs INCLUDE_PARENT)
	target_include_directories(${star_lib_name} PRIVATE ${star_lib_subdirs} ${${star_lib_name}_INCLUDE_DIRECTORIES})
	set_target_properties(${star_lib_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${star_lib_dir_out})

	install(TARGETS ${star_lib_name}
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL
		ARCHIVE DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL)

endfunction()


# Builds a ${star_lib_name}_Tables library from ${star_lib_dir}/idl/*.idl files
function(STAR_ADD_LIBRARY_TABLE star_lib_dir )

	star_target_paths(${star_lib_dir} star_lib_name star_lib_dir_abs star_lib_dir_out)

	set(star_lib_name ${star_lib_name}_Tables)

	file(GLOB_RECURSE idl_files ${star_lib_dir_abs}/idl/*.idl)
	FILTER_LIST( idl_files EXCLUDE ${${star_lib_name}_EXCLUDE} )
	star_process_idl("${idl_files}" "" ${star_lib_dir_out} sources_idl headers_idl)

	add_library(${star_lib_name} ${sources_idl})
	set_target_properties(${star_lib_name} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${star_lib_dir_out}
		PUBLIC_HEADER "${headers_idl}")

	install(TARGETS ${star_lib_name}
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL
		ARCHIVE DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL
		PUBLIC_HEADER DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/include/tables" OPTIONAL)
endfunction()


# Build customized library StGenericVertexMakerNoSti based on StGenericVertexMaker
function(STAR_ADD_LIBRARY_VERTEXNOSTI star_lib_dir )

	star_target_paths(${star_lib_dir} star_lib_name star_lib_dir_abs star_lib_dir_out)

	string(REPLACE ${star_lib_name} StGenericVertexMakerNoSti star_lib_dir_out ${star_lib_dir_out})
	set(vtxnosti_headers
		${star_lib_dir_abs}/StGenericVertexMaker.h
		${star_lib_dir_abs}/Minuit/St_VertexCutsC.h)
	set(linkdef_file "${star_lib_dir_out}_LinkDef.h")
	set(dictinc_file "${star_lib_dir_out}_DictInc.h")

	add_library(StGenericVertexMakerNoSti SHARED
		${star_lib_dir_abs}/StCtbUtility.cxx
		${star_lib_dir_abs}/StFixedVertexFinder.cxx
		${star_lib_dir_abs}/StGenericVertexFinder.cxx
		${star_lib_dir_abs}/StGenericVertexMaker.cxx
		${star_lib_dir_abs}/StppLMVVertexFinder.cxx
		${star_lib_dir_abs}/VertexFinderOptions.cxx
		${star_lib_dir_abs}/Minuit/StMinuitVertexFinder.cxx
		${star_lib_dir_abs}/Minuit/St_VertexCutsC.cxx
		${star_lib_dir_out}_dict.cxx )
	# Output the library to the respecitve subdirectory in the binary directory
	set_target_properties(StGenericVertexMakerNoSti PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${star_lib_dir_out})

	add_custom_command(
		OUTPUT ${dictinc_file} ${linkdef_file}
		COMMAND ${STAR_CMAKE_DIR}/gen_linkdef.sh -l ${linkdef_file} -d ${dictinc_file} ${vtxnosti_headers}
		DEPENDS ${STAR_CMAKE_DIR}/gen_linkdef.sh ${vtxnosti_headers} )

	get_property(global_include_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
	string( REGEX REPLACE "([^;]+)" "-I\\1" global_include_dirs "${global_include_dirs}" )

	# Generate ROOT dictionary using the *_LinkDef.h and *_DictInc.h files
	add_custom_command(
		OUTPUT ${star_lib_dir_out}_dict.cxx
		COMMAND ${ROOT_DICTGEN_EXECUTABLE} -cint -f ${star_lib_dir_out}_dict.cxx
		-c -p -D__ROOT__ ${global_include_dirs}
		${vtxnosti_headers} ${dictinc_file} ${linkdef_file}
		DEPENDS ${vtxnosti_headers} ${dictinc_file} ${linkdef_file}
		VERBATIM )

	install(TARGETS StGenericVertexMakerNoSti
		LIBRARY DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL
		ARCHIVE DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/lib" OPTIONAL)
endfunction()


# Generate source from idl files
function(STAR_PROCESS_IDL idl_files star_lib_name star_lib_dir_out out_sources_idl out_headers_idl)

	set(out_sources_idl_)
	set(out_headers_idl_)

	set(outpath_table_struct ${CMAKE_CURRENT_BINARY_DIR}/include)
	set(outpath_table_ttable ${CMAKE_CURRENT_BINARY_DIR}/include/tables/${star_lib_name})

	foreach( idll ${idl_files} )

		file(STRINGS ${idll} idl_matches_module REGEX "interface.*:.*amiModule")

		if( idl_matches_module )
			star_process_idl_module(${idll} sources_idl_ headers_idl_)
		else()
			star_process_idl_file(${idll} sources_idl_ headers_idl_)
		endif()

		list(APPEND out_sources_idl_ ${sources_idl_})
		list(APPEND out_headers_idl_ ${headers_idl_})
	endforeach()

	set(${out_sources_idl} ${out_sources_idl_} PARENT_SCOPE)
	set(${out_headers_idl} ${out_headers_idl_} PARENT_SCOPE)
endfunction()



function(STAR_PROCESS_IDL_FILE idll out_sources out_headers)

	find_program(PERL_EXECUTABLE perl)

	# For the file and variable names we closely follow the convention in mgr/Conscript-standard
	get_filename_component(idl ${idll} NAME_WE)
	set(idlh "${outpath_table_struct}/${idl}.h")
	set(idli "${outpath_table_struct}/${idl}.inc")
	set(idlH "${outpath_table_ttable}/St_${idl}_Table.h")
	set(idlC "${star_lib_dir_out}/St_${idl}_Table.cxx")
	set(idlLinkDef "${star_lib_dir_out}/${idl}LinkDef.h")
	set(idlCintH "${star_lib_dir_out}/St_${idl}_TableCint.h")
	set(idlCintC "${star_lib_dir_out}/St_${idl}_TableCint.cxx")


	add_custom_command(
		OUTPUT ${idlh} ${idli}
		COMMAND ${CMAKE_CURRENT_BINARY_DIR}/stic -q ${idll}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${outpath_table_struct}
		COMMAND ${CMAKE_COMMAND} -E rename ${idl}.h ${idlh}
		COMMAND ${CMAKE_COMMAND} -E rename ${idl}.inc ${idli}
		DEPENDS ${idll} stic
		VERBATIM )

	add_custom_command(
		OUTPUT ${idlH} ${idlC} ${idlLinkDef}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${outpath_table_ttable}
		COMMAND ${PERL_EXECUTABLE} ${STAR_SRC}/mgr/ConstructTable.pl ${idll} ${idlH}
		COMMAND ${PERL_EXECUTABLE} ${STAR_SRC}/mgr/ConstructTable.pl ${idll} ${idlC}
		COMMAND ${PERL_EXECUTABLE} ${STAR_SRC}/mgr/ConstructTable.pl ${idll} ${idlLinkDef}
		DEPENDS ${STAR_SRC}/mgr/ConstructTable.pl ${idll}
		VERBATIM )

	add_custom_command(
		OUTPUT ${idlCintC} ${idlCintH}
		COMMAND ${ROOT_DICTGEN_EXECUTABLE} -cint -f ${idlCintC} -c -p -D__ROOT__
		-I${outpath_table_struct} -I${outpath_table_ttable} ${idlh} ${idlH} ${idlLinkDef}
		DEPENDS ${idlh} ${idlH} ${idlLinkDef}
		VERBATIM )

	set(${out_sources} ${idlC} ${idlCintC} PARENT_SCOPE)
	set(${out_headers} ${idlH} PARENT_SCOPE)
endfunction()



function(STAR_PROCESS_IDL_MODULE idll out_sources out_headers)

	# For the file and variable names we closely follow the convention in mgr/Conscript-standard
	get_filename_component(idl ${idll} NAME_WE)
	set(idlh "${outpath_table_struct}/${idl}.h")
	set(idli "${outpath_table_struct}/${idl}.inc")
	set(modH "${outpath_table_ttable}/St_${idl}_Module.h")
	set(modC "${star_lib_dir_out}/St_${idl}_Module.cxx")
	set(idlLinkDef "${star_lib_dir_out}/${idl}LinkDef.h")
	set(idlCintH "${star_lib_dir_out}/St_${idl}_ModuleCint.h")
	set(idlCintC "${star_lib_dir_out}/St_${idl}_ModuleCint.cxx")

	add_custom_command(
		OUTPUT ${idlh} ${idli} ${modH} ${modC} ${idlLinkDef}
		COMMAND ${CMAKE_CURRENT_BINARY_DIR}/stic -s -r -q ${idll}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${star_lib_dir_out}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${outpath_table_struct}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${outpath_table_ttable}
		COMMAND ${CMAKE_COMMAND} -E rename St_${idl}_Module.h ${modH}
		COMMAND ${CMAKE_COMMAND} -E rename St_${idl}_Module.cxx ${modC}
		COMMAND ${CMAKE_COMMAND} -E rename ${idl}.h ${idlh}
		COMMAND ${CMAKE_COMMAND} -E rename ${idl}.inc ${idli}
		COMMAND ${CMAKE_COMMAND} -E remove ${idl}.c.template ${idl}.F.template ${idl}_i.cc
		COMMAND ${CMAKE_COMMAND} -E echo "#pragma link C++ class St_${idl}-;" > ${idlLinkDef}
		DEPENDS ${idll} stic
		VERBATIM )

	add_custom_command(
		OUTPUT ${idlCintC} ${idlCintH}
		COMMAND ${ROOT_DICTGEN_EXECUTABLE} -cint -f ${idlCintC} -c -p -D__ROOT__
		-I${outpath_table_struct} -I${outpath_table_ttable} ${modH} ${idlLinkDef}
		DEPENDS ${modH} ${idlLinkDef}
		VERBATIM )

	set(${out_sources} ${modC} ${idlCintC} PARENT_SCOPE)
	set(${out_headers} ${modH} PARENT_SCOPE)
endfunction()



function(STAR_PROCESS_F star_lib_name in_F_files star_lib_dir_out out_F_files)

	set(out_F_files_)

	get_property(global_include_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
	set(target_include_dirs ${global_include_dirs} ${${star_lib_name}_INCLUDE_DIRECTORIES})
	string( REGEX REPLACE "([^;]+)" "-I\\1" target_include_dirs "${target_include_dirs}" )

	foreach( f_file ${in_F_files} )
		get_filename_component(f_file_name_we ${f_file} NAME_WE)
		set(g_file "${star_lib_dir_out}/${f_file_name_we}.g")
		set(out_F_file "${star_lib_dir_out}/${f_file_name_we}.F")

		add_custom_command(
			OUTPUT ${g_file} ${out_F_file}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${star_lib_dir_out}
			COMMAND ${CMAKE_C_COMPILER} -E -P ${STAR_Fortran_DEFINITIONS} ${target_include_dirs} ${f_file} -o ${g_file}
			COMMAND ${CMAKE_CURRENT_BINARY_DIR}/agetof -V 1 ${g_file} -o ${out_F_file}
			DEPENDS ${f_file} agetof VERBATIM)

		list(APPEND out_F_files_ ${out_F_file})
	endforeach()

	set(${out_F_files} ${out_F_files_} PARENT_SCOPE)
endfunction()



function(STAR_PROCESS_G in_g_files star_lib_dir_out out_F_files)

	set(out_F_files_)

	foreach( g_file ${in_g_files} )
		get_filename_component(g_file_name_we ${g_file} NAME_WE)
		set(out_F_file "${star_lib_dir_out}/${g_file_name_we}.F")

		add_custom_command(
			OUTPUT ${out_F_file}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${star_lib_dir_out}
			COMMAND ${CMAKE_CURRENT_BINARY_DIR}/agetof -V 1 ${g_file} -o ${out_F_file}
			DEPENDS ${g_file} agetof VERBATIM)

		list(APPEND out_F_files_ ${out_F_file})
	endforeach()

	set( ${out_F_files} ${out_F_files_} PARENT_SCOPE )
endfunction()



function(STAR_ADD_EXECUTABLE_ROOT4STAR star_exec_dir)

	star_target_paths(${star_exec_dir} dummy exec_dir_abs exec_dir_out)

	add_executable(root4star
		${exec_dir_abs}/MAIN_rmain.cxx
		${exec_dir_abs}/df.F
		${exec_dir_abs}/TGeant3/StarMC.cxx
		${exec_dir_abs}/TGeant3/TGiant3.cxx
		${exec_dir_abs}/TGeant3/gcadd.cxx
		${exec_dir_abs}/TGeant3/galicef.F
		${exec_dir_abs}/TGeant3/gcomad.F
		${exec_dir_out}/rexe_DictInc.cxx
	)

	target_include_directories(root4star PRIVATE
		"${STAR_SRC}/asps/Simulation/geant321/include")

	add_custom_command(
		OUTPUT ${exec_dir_out}/rexe_DictInc.cxx
		COMMAND ${CMAKE_COMMAND} -E make_directory ${exec_dir_out}
		COMMAND ${ROOT_DICTGEN_EXECUTABLE} -cint -f  ${exec_dir_out}/rexe_DictInc.cxx -c -p -D__ROOT__
		-I${exec_dir_abs}/TGeant3/ StarMC.h TGiant3.h ${exec_dir_abs}/rexeLinkDef.h
		DEPENDS  ${exec_dir_abs}/TGeant3/StarMC.h ${exec_dir_abs}/TGeant3/TGiant3.h ${exec_dir_abs}/rexeLinkDef.h
		VERBATIM )

	target_link_libraries(root4star starsimlib geant321 starsimlib gcalor
		${ROOT_LIBRARIES} ${CERNLIB_LIBRARIES} gfortran Xt Xpm X11 Xm dl crypt)

	install(TARGETS root4star
        RUNTIME DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/bin" OPTIONAL)
endfunction()



function(STAR_ADD_EXECUTABLE_AGETOF star_exec_dir)

	star_target_paths(${star_exec_dir} dummy exec_dir_abs dummy)

	add_executable( agetof
		${exec_dir_abs}/readlink.c
		${exec_dir_abs}/afexist.F
		${exec_dir_abs}/afname.F
		${exec_dir_abs}/cccard.F
		${exec_dir_abs}/crit.F
		${exec_dir_abs}/define.F
		${exec_dir_abs}/doit.F
		${exec_dir_abs}/dumdum.F
		${exec_dir_abs}/expand.F
		${exec_dir_abs}/filter.F
		${exec_dir_abs}/find.F
		${exec_dir_abs}/inital.F
		${exec_dir_abs}/ispec.F
		${exec_dir_abs}/kat.F
		${exec_dir_abs}/krunc.F
		${exec_dir_abs}/lexp.F
		${exec_dir_abs}/linf.F
		${exec_dir_abs}/llong.F
		${exec_dir_abs}/mactrc.F
		${exec_dir_abs}/main.F
		${exec_dir_abs}/mesage.F
		${exec_dir_abs}/muser.F
		${exec_dir_abs}/n5.F
		${exec_dir_abs}/nib.F
		${exec_dir_abs}/nxtcrd.F
		${exec_dir_abs}/raw.F
		${exec_dir_abs}/rw.F
		${exec_dir_abs}/user.F
	)

	add_custom_command(TARGET agetof PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${exec_dir_abs}/agetof.def ${CMAKE_CURRENT_BINARY_DIR}
		VERBATIM)

	target_link_libraries(agetof ${ROOT_LIBRARIES})
endfunction()



function(STAR_ADD_EXECUTABLE_STIC star_exec_dir)

	star_target_paths(${star_exec_dir} orig_exec_name exec_dir_abs exec_dir_out)

	string(REPLACE ${orig_exec_name} stic exec_dir_out ${exec_dir_out})

	add_executable(stic ${exec_dir_out}/idl-yacc.c ${exec_dir_abs}/templateStuff.c)
	target_include_directories(stic PRIVATE ${exec_dir_abs} ${exec_dir_out})
	target_link_libraries(stic fl)

	find_program(BISON_EXECUTABLE NAMES bison yacc)
	add_custom_command(
		OUTPUT ${exec_dir_out}/idl-yacc.c
		COMMAND ${CMAKE_COMMAND} -E make_directory ${exec_dir_out}
		COMMAND ${BISON_EXECUTABLE} -o ${exec_dir_out}/idl-yacc.c ${exec_dir_abs}/idl.y
		DEPENDS ${exec_dir_abs}/idl.y ${exec_dir_abs}/stic.h ${exec_dir_out}/idl-lex.c
		VERBATIM)

	find_program(FLEX_EXECUTABLE flex)
	add_custom_command(
		OUTPUT ${exec_dir_out}/idl-lex.c
		COMMAND ${CMAKE_COMMAND} -E make_directory ${exec_dir_out}
		COMMAND ${FLEX_EXECUTABLE} -o ${exec_dir_out}/idl-lex.c ${exec_dir_abs}/idl.l
		DEPENDS ${exec_dir_abs}/idl.l
		VERBATIM)
endfunction()


#
# Some targets require specific treatment due to missing functionality
# originally available in cons
#
set(St_base_LINKDEF_HEADERS "${STAR_SRC}/StRoot/St_base/Stypes.h")
set(StAnalysisUtilities_LINKDEF_HEADERS "$ENV{ROOTSYS}/include/TFile.h")
set(StBFChain_LINKDEF_HEADERS "$ENV{ROOTSYS}/include/TFile.h")
set(StEvent_LINKDEF_HEADERS
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2000.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2002.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2003.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2004.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2005.h"
	"${STAR_SRC}/StRoot/StDaqLib/TRG/trgStructures2007.h"
	"${STAR_CMAKE_DIR}/star-aux/StArray_cint.h")
set(StEStructPool_LINKDEF_HEADERS
	"$ENV{ROOTSYS}/include/TVector2.h"
	"${STAR_CMAKE_DIR}/star-aux/StArray_cint.h")
set(StGammaMaker_LINKDEF_HEADERS "$ENV{ROOTSYS}/include/TVector3.h")
set(StTriggerUtilities_LINKDEF_HEADERS "${STAR_SRC}/StRoot/StChain/StMaker.h")
set(StiMaker_LINKDEF_HEADERS "$ENV{ROOTSYS}/include/TH1K.h")
set(Stv_LINKDEF_HEADERS "${STAR_SRC}/StRoot/StarRoot/THelixTrack.h")

set(St_g2t_INCLUDE_DIRECTORIES
	"${STAR_SRC}/asps/Simulation/geant321/include"
	"${STAR_SRC}/asps/Simulation/starsim/include")
set(geant321_INCLUDE_DIRECTORIES
	"${STAR_SRC}/asps/Simulation/starsim/include")
set(gcalor_INCLUDE_DIRECTORIES
	"${STAR_SRC}/asps/Simulation/geant321/include"
	"${STAR_SRC}/asps/Simulation/starsim/include")

set(St_base_EXCLUDE "StRoot/St_base/St_staf_dummies.c")
set(StDb_Tables_EXCLUDE "StDb/idl/tpcDedxPidAmplDb.idl")
set(StarMiniCern_EXCLUDE
	"StarVMC/minicern/allgs"
	"StarVMC/minicern/hpxgs"
	"StarVMC/minicern/lnxgs"
	"StarVMC/minicern/lnxppcgs"
	"StarVMC/minicern/kerngen"
	"StarVMC/minicern/qutyinv"
	"StarVMC/minicern/qutyz32"
	"StarVMC/minicern/sungs")
set(geant321_EXCLUDE
	"asps/Simulation/geant321/doc"
	"asps/Simulation/geant321/gxuser")
