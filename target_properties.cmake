# Manually set package specific properties

include(StarCommon)


if(TARGET StGenericVertexMaker)
	# Common build rules cannot be applied to StGenericVertexMakerNoSti library
	star_add_library_vertexnosti( StRoot/StGenericVertexMaker )
	add_dependencies(StGenericVertexMakerNoSti StDb_Tables geometry_Tables sim_Tables)
endif()


if(TARGET TPCCATracker)
	set_target_properties(TPCCATracker PROPERTIES LINK_LIBRARIES "${ROOT_Vc_LIBRARY}")
endif()


if(TARGET Stv)
	target_include_directories(Stv PRIVATE "${STAR_CMAKE_DIR}/star-aux/StarVMC/geant3/TGeant3")
endif()
