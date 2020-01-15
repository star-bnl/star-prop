define_property(SOURCE PROPERTY AGETOF_ADDITIONAL_OPTIONS
	BRIEF_DOCS "Additional options for agetof executable"
	FULL_DOCS  "Some Fortran files require agetof to take the '-V f' option in "
	           "order to generate a correct fortran file from the preprocessed one")

# g2t: Note "-V f" is without quotes due to VERBATIM option in the correspongin
# ADD_CUSTOM_COMMAND() where this values is used
file(GLOB f_files "${STAR_SRC}/pams/sim/g2t/*.F")
foreach(f_file ${f_files})
	set_property(SOURCE ${f_file} PROPERTY AGETOF_ADDITIONAL_OPTIONS -V f)
	set_property(SOURCE ${f_file} PROPERTY INCLUDE_DIRECTORIES
		"${STAR_SRC}/asps/Simulation/geant321/include"
		"${STAR_SRC}/asps/Simulation/starsim/include/"
		"${CMAKE_CURRENT_BINARY_DIR}/include")
endforeach()

set_property(SOURCE "${STAR_SRC}/pams/sim/g2t/g2r_get.F" PROPERTY AGETOF_ADDITIONAL_OPTIONS )

# StgMaker: Add include directory for ROOT dictionary generation with installed
# external dependencies
set_property(SOURCE "${PROJECT_SOURCE_DIR}/StRoot/StgMaker" PROPERTY INCLUDE_DIRECTORIES "${STAR_INSTALL_INCLUDEDIR}")
