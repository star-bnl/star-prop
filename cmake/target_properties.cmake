# Set package specific properties

if(TARGET RTS)
	set(_inc_dirs
		${ROOT_INCLUDE_DIR}
		${STAR_SRC}/StRoot
		${STAR_SRC}/StRoot/RTS/src
		${STAR_SRC}/StRoot/RTS/include
		${STAR_SRC}/StRoot/RTS/trg/include)
	set_target_properties(RTS PROPERTIES INCLUDE_DIRECTORIES "${_inc_dirs}")
endif()

if(TARGET StBTofHitMaker)
	target_include_directories(StBTofHitMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src")
endif()

if(TARGET StDaqLib)
	target_include_directories(StDaqLib PRIVATE "${STAR_SRC}/StRoot/RTS/include")
	target_compile_options(StDaqLib PRIVATE -DNEW_DAQ_READER)
endif()

if(TARGET StDAQMaker)
	target_include_directories(StDAQMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src;${STAR_SRC}/StRoot/RTS/include")
endif()

if(TARGET StEmcRawMaker)
	target_include_directories(StEmcRawMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src")
endif()

if(TARGET StFgtRawMaker)
	target_include_directories(StFgtRawMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src;${STAR_SRC}/StRoot/RTS/include")
endif()

if(TARGET StFpsRawHitMaker)
	target_include_directories(StFpsRawHitMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src;${STAR_SRC}/StRoot/RTS/include")
endif()

if(TARGET StRtsReaderMaker)
	target_include_directories(StRtsReaderMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src")
endif()

# Common build rules cannot be applied to StGenericVertexMakerNoSti library
if(TARGET StGenericVertexMaker)
	star_add_library_vertexnosti(StRoot/StGenericVertexMaker)
	add_dependencies(StGenericVertexMakerNoSti StDb_Tables geometry_Tables sim_Tables)
endif()

if(TARGET StIstRawHitMaker)
	target_include_directories(StIstRawHitMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src;${STAR_SRC}/StRoot/RTS/include")
endif()

# -D_UCMLOGGER_ is used in StStarLogger
if(TARGET StStarLogger)
	target_compile_options(StStarLogger PRIVATE -D_UCMLOGGER_)
	target_include_directories(StStarLogger PRIVATE "${LOG4CXX_INCLUDE_DIR};${MYSQL_INCLUDE_DIRS}")
	set_target_properties(StStarLogger PROPERTIES LINK_LIBRARIES "${LOG4CXX_LIBRARIES}")
	set_target_properties(StStarLogger PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

if(TARGET TPCCATracker)
	set_target_properties(TPCCATracker PROPERTIES LINK_LIBRARIES "${ROOT_Vc_LIBRARY}")
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink libTPCCATracker.so \
		${CMAKE_INSTALL_PREFIX}/${STAR_ADDITIONAL_INSTALL_PREFIX}/lib/TPCCATracker.so)")
endif()

if(TARGET StiCA)
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink libStiCA.so \
		${CMAKE_INSTALL_PREFIX}/${STAR_ADDITIONAL_INSTALL_PREFIX}/lib/StiCA.so)")
endif()

if(TARGET StMtdHitMaker)
	target_include_directories(StMtdHitMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src")
endif()

if(TARGET StTpcHitMaker)
	target_include_directories(StTpcHitMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src;${STAR_SRC}/StRoot/RTS/include")
endif()

if(TARGET Stv)
	target_include_directories(Stv PRIVATE "${STAR_SRC}/StarVMC/geant3/TGeant3")
endif()

if(TARGET StvMaker)
	target_include_directories(StvMaker PRIVATE "${STAR_SRC}")
endif()

if(TARGET StvSeed)
	target_include_directories(StvSeed PRIVATE "${STAR_SRC}")
endif()

if(TARGET StvUtil)
	target_include_directories(StvUtil PRIVATE "${STAR_SRC}")
endif()

if(TARGET St_g2t)
	target_include_directories(St_g2t PRIVATE "${STAR_SRC}/asps/Simulation/geant321/include"
	                                          "${STAR_SRC}/asps/Simulation/starsim/include")
endif()

# CERNLIB_CG is used by asps/Simulation/geant321/gdraw/gdcota.F
# CERNLIB_COMIS is used by asps/Simulation/geant321/gxint/gxcs.F
if(TARGET geant321)
	target_include_directories(geant321 PRIVATE "${STAR_SRC}/asps/Simulation/starsim/include")
	target_compile_options(geant321 PRIVATE
		-DCERNLIB_BSLASH
		-DCERNLIB_CG
		-DCERNLIB_COMIS
		-DCERNLIB_CZ
		-DCERNLIB_DOUBLE
		-DCERNLIB_DZDOC
		-DCERNLIB_HIGZ
		-DCERNLIB_LINUX
		-DCERNLIB_TYPE)
endif()

# CPP_DATE, CPP_TIME, CPP_TITLE_CH, and CPP_VERS are used by gcalor library
if(TARGET gcalor)
	target_include_directories(gcalor PRIVATE "${STAR_SRC}/asps/Simulation/geant321/include"
	                                          "${STAR_SRC}/asps/Simulation/starsim/include")
	target_compile_options(gcalor PRIVATE
		-DCPP_DATE=${STAR_BUILD_DATE} -DCPP_TIME=${STAR_BUILD_TIME} -DCPP_TITLE_CH="gcalor" -DCPP_VERS="W")
endif()

# CPP_DATE, CPP_TIME, CPP_TITLE_CH, and CPP_VERS are used by starsim library
# (i.e. asps/Simulation/starsim/aversion.F in starsimlib)
# GFORTRAN is used by asps/Simulation/starsim/atmain/etime.F
# f2cFortran is used by starsim and cern/pro/include/cfortran/cfortran.h
if(TARGET starsimlib)
	target_compile_options(starsimlib PRIVATE
		-DCPP_DATE=${STAR_BUILD_DATE} -DCPP_TIME=${STAR_BUILD_TIME} -DCPP_TITLE_CH="starsim" -DCPP_VERS="W"
		-DGFORTRAN -Df2cFortran -DWithoutPGI
		-DCERNLIB_BSLASH
		-DCERNLIB_CG
		-DCERNLIB_DZDOC
		-DCERNLIB_GCALOR
		-DCERNLIB_HADRON
		-DCERNLIB_HIGZ
		-DCERNLIB_LINUX
		-DCERNLIB_MYSQL
		-DCERNLIB_NONEWL
		-DCERNLIB_SHL
		-DCERNLIB_TYPE)
endif()

if(TARGET StEpcMaker)
	set_target_properties(StEpcMaker PROPERTIES LINK_LIBRARIES "${CERNLIB_LIBRARIES}")
endif()

if(TARGET StDbBroker)
	target_include_directories(StDbBroker PRIVATE "${MYSQL_INCLUDE_DIRS}")
	set_target_properties(StDbBroker PROPERTIES LINK_LIBRARIES "${MYSQL_LIBRARIES}")
	get_filename_component(_mysql_dir ${MYSQL_LIBRARIES} DIRECTORY)
	set_target_properties(StDbBroker PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${STAR_ADDITIONAL_INSTALL_PREFIX}/lib;${_mysql_dir}")
endif()

if(TARGET StDbLib)
	target_include_directories(StDbLib PRIVATE "${LIBXML2_INCLUDE_DIR};${MYSQL_INCLUDE_DIRS}")
	set_target_properties(StDbLib PROPERTIES LINK_LIBRARIES "${LIBXML2_LIBRARIES};${MYSQL_LIBRARIES}")
	get_filename_component(_libxml2_dir ${LIBXML2_LIBRARIES} DIRECTORY)
	get_filename_component(_mysql_dir ${MYSQL_LIBRARIES} DIRECTORY)
	set_target_properties(StDbLib PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${STAR_ADDITIONAL_INSTALL_PREFIX}/lib;${_libxml2_dir};${_mysql_dir}")
endif()

if(TARGET StarAgmlUtil)
	target_include_directories(StarAgmlUtil PRIVATE "${STAR_SRC}")
endif()

if(TARGET StarClassLibrary)
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink libStarClassLibrary.so \
		${CMAKE_INSTALL_PREFIX}/${STAR_ADDITIONAL_INSTALL_PREFIX}/lib/StarClassLibrary.so)")
endif()

if(TARGET StDbLib)
	target_include_directories(StDbLib PRIVATE "${LIBXML2_INCLUDE_DIR}")
	set_target_properties(StDbLib PROPERTIES LINK_LIBRARIES "${LIBXML2_LIBRARIES}")
endif()

if(TARGET StarMagFieldNoDict)
	target_compile_options(StarMagFieldNoDict PRIVATE "-U__ROOT__")
endif()

if(TARGET StSstDaqMaker)
	target_include_directories(StSstDaqMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src;${STAR_SRC}/StRoot/RTS/include")
endif()

if(TARGET StTofHitMaker)
	target_include_directories(StTofHitMaker PRIVATE "${STAR_SRC}/StRoot/RTS/src")
	target_compile_options(StTofHitMaker PRIVATE -DNEW_DAQ_READER)
endif()

if(TARGET xgeometry)
	set_target_properties(xgeometry PROPERTIES LINK_LIBRARIES StarMagFieldNoDict)
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink libxgeometry.so \
		${CMAKE_INSTALL_PREFIX}/${STAR_ADDITIONAL_INSTALL_PREFIX}/lib/xgeometry.so)")
endif()
