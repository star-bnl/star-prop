# Manually set package specific properties


if(TARGET StGenericVertexMaker)
	# Common build rules cannot be applied to StGenericVertexMakerNoSti library
	star_add_library_vertexnosti( StRoot/StGenericVertexMaker )
	add_dependencies(StGenericVertexMakerNoSti StDb_Tables geometry_Tables sim_Tables)
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


if(TARGET geant321)
	target_include_directories(geant321 PRIVATE "${STAR_SRC}/asps/Simulation/starsim/include")
endif()


if(TARGET gcalor)
	target_include_directories(gcalor PRIVATE "${STAR_SRC}/asps/Simulation/geant321/include"
	                                          "${STAR_SRC}/asps/Simulation/starsim/include")
endif()

if(TARGET StEpcMaker)
    set_target_properties(StEpcMaker PROPERTIES LINK_LIBRARIES "${CERNLIB_LIBRARIES}")
endif()
