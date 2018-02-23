cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(star-cvs)

set(STAR_SRC "$ENV{STAR}" CACHE PATH "Path to star software source code")

# Add to path in order to pick up the FindXXX.cmake files included in this project
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

# STAR software depends on ROOT
find_package(ROOT COMPONENTS Table HistPainter Minuit Geom Spectrum)

if( ${ROOT_CXX_FLAGS} MATCHES "-m32" )
	message( STATUS "Found -m32 option in $ROOT_CXX_FLAGS (root-config). Will add it to $CMAKE_CXX_FLAGS" )
	set_property( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS FALSE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" )
endif()

find_package(LibXml2)
find_package(Log4Cxx)
find_package(MySQL)

# Perform some setup standard to STAR experiment environment
include(StarCommon)

add_definitions( -D__ROOT__ )
add_definitions( -D_UCMLOGGER_ ) # Required by StStarLogger

# Remove dependency of "install" target on "all" target. This allows to
# build and install individual libraries
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

star_preinstall_headers( StarVMC )
star_preinstall_headers( StRoot )

include_directories(
	${ROOT_INCLUDE_DIR}
	${LIBXML2_INCLUDE_DIR}
	${LOG4CXX_INCLUDE_DIR}
	${MYSQL_INCLUDE_DIRS}
	${STAR_SRC}
	${STAR_SRC}/StRoot
	${STAR_SRC}/StRoot/RTS/include
	${STAR_SRC}/StRoot/RTS/src
	${STAR_SRC}/StRoot/RTS/trg/include
	${CMAKE_BINARY_DIR}/${STAR_ADDITIONAL_INSTALL_PREFIX}/include
	# The following directories need to be included until header files can
	# be generated from idl
	$ENV{STAR}/.$ENV{STAR_HOST_SYS}/include/tables
	$ENV{STAR}/.$ENV{STAR_HOST_SYS}/include
)


# It may make sense to try to invert this whitelist to create a blacklist
# instead
set( star_lib_dirs
	#StRoot/StBTofPool
	#StRoot/StEEmcPool
	#StRoot/StEmcPool
	#StRoot/StEpdDbMaker
	#StRoot/StFgtA2CMaker
	#StRoot/StFgtClusterMaker
	#StRoot/StFgtDbMaker
	#StRoot/StFgtPointMaker
	#StRoot/StFgtPool
	#StRoot/StFgtRawMaker
	#StRoot/StFgtSimulatorMaker
	#StRoot/StFgtUtil
	#StRoot/StFtpcCalibMaker
	#StRoot/StFtpcClusterMaker
	#StRoot/StFtpcDriftMapMaker
	#StRoot/StFtpcMixerMaker
	#StRoot/StFtpcSlowSimMaker
	#StRoot/StFtpcTrackMaker
	#StRoot/StHbtMaker
	#StRoot/StRichPool
	#StRoot/StSvtPool
	#StRoot/StTofHitMaker
	#StRoot/StTofPool
	#StRoot/StTofSimMaker
	#StRoot/StTofpMatchMaker
	#StRoot/StTofrMatchMaker
	#StRoot/StarGenerator
	#StRoot/StJetMaker
	#StRoot/StSpinPool
	#StRoot/StTagsMaker  # requires idl processing
	#StarVMC/StarGeometry  # requires some preprocessing
	#
	StarVMC/Geometry
	StarVMC/StarAgmlChecker
	StarVMC/StarAgmlLib
	StarVMC/StarAgmlUtil
	StarVMC/StarAgmlViewer
	#
	StRoot/StAnalysisMaker
	StRoot/StAnalysisUtilities
	StRoot/Star2Root
	StRoot/StarClassLibrary
	StRoot/StarMagField
	StRoot/StarRoot
	StRoot/StAssociationMaker
	StRoot/St_base
	StRoot/St_baseTest
	StRoot/StBbcSimulationMaker
	StRoot/StBeamBackMaker
	StRoot/StBFChain
	StRoot/StBichsel
	StRoot/StBTofCalibMaker
	StRoot/StBTofHitMaker
	StRoot/StBTofMatchMaker
	StRoot/StBTofMixerMaker
	StRoot/StBTofSimMaker
	StRoot/StBTofUtil
	StRoot/StChain
	StRoot/St_ctf_Maker
	StRoot/StDaqLib
	StRoot/StDAQMaker
	StRoot/StDataFilterMaker
	StRoot/StDbBroker
	StRoot/StDbLib
	StRoot/St_db_Maker
	StRoot/StDbUtilities
	StRoot/StdEdxY2Maker
	StRoot/StDetectorDbMaker
	StRoot/StEEmcDbMaker
	StRoot/StEEmcSimulatorMaker
	StRoot/StEEmcUtil
	StRoot/StEmbeddingUtilities
	StRoot/StEmcADCtoEMaker
	StRoot/StEmcCalibrationMaker
	StRoot/St_emc_Maker
	StRoot/StEmcMixerMaker
	StRoot/StEmcRawMaker
	StRoot/StEmcSimulatorMaker
	StRoot/StEmcTriggerMaker
	StRoot/StEmcUtil
	StRoot/StEpcMaker
	StRoot/StEpdDbMaker
	StRoot/StEvent
	StRoot/StEventCompendiumMaker
	StRoot/StEventMaker
	StRoot/StEventUtilities
	StRoot/StFilterMaker
	StRoot/StFlowAnalysisMaker
	StRoot/StFmsDbMaker
	StRoot/StFmsFastSimulatorMaker
	StRoot/StFmsFpsMaker
	StRoot/StFmsHitMaker
	StRoot/StFmsPointMaker
	StRoot/StFmsUtil
	StRoot/StGammaMaker
	StRoot/StGenericVertexMaker
	StRoot/StHeavyTagMaker
	StRoot/StHighPtTagsMaker
	StRoot/Sti
	StRoot/StiCA
	StRoot/StiIst
	StRoot/StiMaker
	StRoot/StIOMaker
	StRoot/StiPxl
	StRoot/StiRnD
	StRoot/StiSsd
	StRoot/StIstClusterMaker
	StRoot/StIstDbMaker
	StRoot/StIstHitMaker
	StRoot/StIstRawHitMaker
	StRoot/StIstSimMaker
	StRoot/StIstUtil
	StRoot/StiSvt
	StRoot/StiTpc
	StRoot/StiUtilities
	StRoot/StJetFinder
	StRoot/Stl3RawReaderMaker
	StRoot/Stl3Util
	StRoot/StLaserAnalysisMaker
	StRoot/StMagF
	StRoot/StMcAnalysisMaker
	StRoot/StMcEvent
	StRoot/StMcEventMaker
	StRoot/StMCFilter
	StRoot/StMcQaMaker
	StRoot/StMiniMcEvent
	StRoot/StMiniMcMaker
	StRoot/StMtdCalibMaker
	StRoot/StMtdEvtFilterMaker
	StRoot/StMtdHitMaker
	StRoot/StMtdMatchMaker
	StRoot/StMtdQAMaker
	StRoot/StMtdSimMaker
	StRoot/StMtdUtil
	StRoot/StMuAnalysisMaker
	StRoot/StMuDSTMaker
	StRoot/StPass0CalibMaker
	StRoot/StPCollTagMaker
	StRoot/StPeCMaker
	StRoot/StPicoDstMaker
	StRoot/StPicoEvent
	StRoot/StPidAmpMaker
	StRoot/StPmdCalibrationMaker
	StRoot/StPmdClusterMaker
	StRoot/StPmdDiscriminatorMaker
	StRoot/StPmdReadMaker
	StRoot/StPmdSimulatorMaker
	StRoot/StPmdUtil
	StRoot/St_pp2pp_Maker
	StRoot/StPreEclMaker
	StRoot/StPxlClusterMaker
	StRoot/StPxlDbMaker
	StRoot/StPxlHitMaker
	StRoot/StPxlRawHitMaker
	StRoot/StPxlSimMaker
	StRoot/StPxlUtil
	StRoot/St_QA_Maker
	StRoot/StRrsMaker
	StRoot/St_sdm_Maker
	StRoot/StSecondaryVertexMaker
	StRoot/St_srs_Maker
	StRoot/StSsdDaqMaker
	StRoot/StSsdDbMaker
	StRoot/StSsdFastSimMaker
	StRoot/StSsdPointMaker
	StRoot/StSsdSimulationMaker
	StRoot/StSsdUtil
	StRoot/StSstDaqMaker
	StRoot/StSstPointMaker
	StRoot/StSstUtil
	StRoot/StStarLogger
	StRoot/StStrangeMuDstMaker
	StRoot/StStrangeTagsMaker
	StRoot/StSvtCalibMaker
	StRoot/StSvtClassLibrary
	StRoot/StSvtClusterMaker
	StRoot/StSvtDaqMaker
	StRoot/StSvtDbMaker
	StRoot/StSvtSeqAdjMaker
	StRoot/StSvtSimulationMaker
	StRoot/StTableUtilities
	StRoot/St_tcl_Maker
	StRoot/St_TLA_Maker
	StRoot/StTofCalibMaker
	StRoot/StTofMaker
	StRoot/StTofpMatchMaker
	StRoot/StTofrMatchMaker
	StRoot/StTofSimMaker
	StRoot/StTofUtil
	StRoot/StTpcDb
	StRoot/StTpcHitMaker
	StRoot/StTpcHitMoverMaker
	StRoot/StTpcRSMaker
	StRoot/StTreeMaker
	StRoot/StTrgDatFileReader
	StRoot/St_trg_Maker
	StRoot/StTrgMaker
	StRoot/StTriggerDataMaker
	StRoot/StTriggerUtilities
	StRoot/StUtilities
	StRoot/Stv
	StRoot/StvMaker
	StRoot/StVpdCalibMaker
	StRoot/StVpdSimMaker
	StRoot/StvSeed
	StRoot/StvUtil
	StRoot/StXTrakMaker
	StRoot/StZdcVertexMaker
	StRoot/TPCCATracker
)


foreach( star_lib_dir ${star_lib_dirs} )
	star_add_library( ${star_lib_dir} )
endforeach()


install(DIRECTORY ${parent_dir} DESTINATION "${STAR_ADDITIONAL_INSTALL_PREFIX}/include"
        MESSAGE_NEVER
        FILES_MATCHING REGEX "\\.(h|hh)$")
