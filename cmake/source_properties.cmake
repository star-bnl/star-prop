define_property(SOURCE PROPERTY AGETOF_ADDITIONAL_OPTIONS
	BRIEF_DOCS "Additional options for agetof executable"
	FULL_DOCS  "Some Fortran files require agetof to take the '-V f' option in "
	           "order to generate a correct fortran file from the preprocessed one")

# Note "-V f" is without quotes due to VERBATIM option in the correspongin
# ADD_CUSTOM_COMMAND() where this values is used
set_property(SOURCE ${STAR_SRC}/pams/sim/g2t/g2t_fts.F PROPERTY AGETOF_ADDITIONAL_OPTIONS -V f)
