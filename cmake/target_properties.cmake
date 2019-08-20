# Set package specific properties

# Common build rules cannot be applied to StGenericVertexMakerNoSti library
if(TARGET StGenericVertexMaker)
	star_add_library_vertexnosti(StRoot/StGenericVertexMaker)
	add_dependencies(StGenericVertexMakerNoSti StDb_Tables geometry_Tables sim_Tables)
endif()

# -D_UCMLOGGER_ is used in StStarLogger
if(TARGET StStarLogger)
	target_compile_options(StStarLogger PRIVATE -D_UCMLOGGER_)
endif()

if(TARGET TPCCATracker)
	set_target_properties(TPCCATracker PROPERTIES LINK_LIBRARIES "${ROOT_Vc_LIBRARY}")
endif()

if(TARGET Stv)
	target_include_directories(Stv PRIVATE "${STAR_SRC}/StarVMC/geant3/TGeant3")
endif()

if(TARGET St_g2t)
	target_include_directories(St_g2t PRIVATE "${STAR_SRC}/asps/Simulation/geant321/include"
	                                          "${STAR_SRC}/asps/Simulation/starsim/include")
endif()

# CERNLIB_CG is used by asps/Simulation/geant321/gdraw/gdcota.F
# CERNLIB_COMIS is used by asps/Simulation/geant321/gxint/gxcs.F
if(TARGET geant321)
	target_include_directories(geant321 PRIVATE "${STAR_SRC}/asps/Simulation/starsim/include")
	target_compile_options(geant321 PRIVATE -DCERNLIB_CG -DCERNLIB_COMIS)
endif()

# CPP_DATE, CPP_TIME, CPP_TITLE_CH, and CPP_VERS are used by gcalor library
if(TARGET gcalor)
	target_include_directories(gcalor PRIVATE "${STAR_SRC}/asps/Simulation/geant321/include"
	                                          "${STAR_SRC}/asps/Simulation/starsim/include")
	target_compile_options(gcalor PRIVATE
		-DCPP_DATE=0 -DCPP_TIME=0 -DCPP_TITLE_CH="dummy" -DCPP_VERS="dummy")
endif()

# CPP_DATE, CPP_TIME, CPP_TITLE_CH, and CPP_VERS are used by starsim library
# (i.e. asps/Simulation/starsim/aversion.F in starsimlib)
# GFORTRAN is used by asps/Simulation/starsim/atmain/etime.F
# f2cFortran is used by starsim and cern/pro/include/cfortran/cfortran.h
if(TARGET starsimlib)
	target_compile_options(starsimlib PRIVATE
		-DCPP_DATE=0 -DCPP_TIME=0 -DCPP_TITLE_CH="dummy" -DCPP_VERS="dummy"
		-DGFORTRAN -Df2cFortran)
endif()

if(TARGET StEpcMaker)
	set_target_properties(StEpcMaker PROPERTIES LINK_LIBRARIES "${CERNLIB_LIBRARIES}")
endif()

if(TARGET StarMagFieldNoDict)
	target_compile_options(StarMagFieldNoDict PRIVATE "-U__ROOT__")
endif()

if(TARGET xgeometry)
	set_target_properties(xgeometry PROPERTIES LINK_LIBRARIES StarMagFieldNoDict)
endif()
