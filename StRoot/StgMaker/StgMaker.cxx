#define LOGURU_IMPLEMENTATION 1

#include "StgMaker/StgMaker.h"
#include "StgMaker/include/Tracker/FwdTracker.h"
#include "StgMaker/include/Tracker/FwdHit.h"
#include "StgMaker/include/Tracker/TrackFitter.h"
#include "StgMaker/XmlConfig/XmlConfig.h"

#include "KiTrack/IHit.h"

#include "TMath.h"

#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "StEvent/StEvent.h"
#include "StEvent/StTrack.h"
#include "StEvent/StTrackNode.h"
#include "StEvent/StGlobalTrack.h"
#include "StEvent/StPrimaryTrack.h"
#include "StEvent/StTrackGeometry.h"
#include "StEvent/StHelixModel.h"
#include "StEvent/StTrackDetectorInfo.h"
#include "StEvent/StEnumerations.h"

#include "tables/St_g2t_track_Table.h"
#include "tables/St_g2t_fts_hit_Table.h"

#include "StarMagField/StarMagField.h"

#include "StEvent/StEnumerations.h"
#include "StEvent/StEvent.h"
#include "StEvent/StPrimaryVertex.h"


#include "StEventUtilities/StEventHelper.h"

#include "StarClassLibrary/StPhysicalHelix.hh"
#include "St_base/StMessMgr.h"

#include <SystemOfUnits.h>
#include <exception>

// For now, accept anything we are passed, no matter what it is or how bad it is
template<typename T> bool accept( T ) { return true; }

// Basic sanity cuts on genfit tracks
template<> bool accept( genfit::Track *track )
{
    // This also gets rid of failed fits (but may need to explicitly
    // for fit failure...)
    if (track->getNumPoints() <= 0 ) return false; // fit may have failed

    auto cardinal = track->getCardinalRep();

    // Check that the track fit converged
    auto status = track->getFitStatus( cardinal );
    if ( 0 == status->isFitConverged() ) {
      return false;
    }


    // Next, check that all points on the track have fitter info
    // (may be another indication of a failed fit?)
    for ( auto point : track->getPoints() ) {
      if ( !point->hasFitterInfo(cardinal) ) {
	return false;
      }
    }

    // Following line fails with an exception, because some tracks lack 
    //   forward update, or prediction in fitter info at the first point
    //
    // genfit::KalmanFitterInfo::getFittedState(bool) const of 
    //                         GenFit/fitters/src/KalmanFitterInfo.cc:250
  
    // Fitted state at the first point
    // const auto &atFirstPoint = track->getFittedState();

    // Getting the fitted state from a track occasionally fails, because
    // the first point on the fit doesn't have forward/backward fit
    // information.  So we want the first point with fit info...
 
    genfit::TrackPoint* first = 0;
    unsigned int ipoint = 0;
    for ( ipoint = 0; ipoint < track->getNumPoints(); ipoint++ ) {
      first = track->getPointWithFitterInfo( ipoint );
      if ( first ) break;
    } 
  
    // No points on the track have fit information
    if ( 0 == first ) {
      LOG_INFO << "No fit information on track" << endm;
      return false;
    }

    auto& fittedState= track->getFittedState(ipoint);

    TVector3 momentum = fittedState.getMom();
    double   pt       = momentum.Perp();

    if (pt < 0.10 ) return false; // below this

    return true;
 
};

//_______________________________________________________________________________________
// // Truth handlers
int TheTruth ( const Seed_t& seed, int &qa ) {

  int count = 0;
  std::map<int,int> truth;
  for ( auto hit : seed ) {
    count++; // add another hit
    FwdHit* fhit = dynamic_cast<FwdHit*>(hit);
    if ( 0 == fhit ) continue;
    truth[ fhit->_tid ]++;
  }

  int id = -1;
  int nmax = -1;
  for (auto const& it : truth ) {
    if ( it.second > nmax ) {
      nmax = it.second;
      id   = it.first;
    }
  }
  // QA is stored as an integer representing the percentage of hits which
  // vote the same way on the track
  qa = int( 100.0 * double(nmax) / double(count) );
  return id;

};

// Adaptor for STAR magnetic field
class StarFieldAdaptor : public genfit::AbsBField
{
public:

  StarFieldAdaptor()
  {
    _gField = this; // global pointer in TrackFitter.h
  };

  virtual TVector3 get(const TVector3 &position) const
  {
    double x[] = { position[0], position[1], position[2] };
    double B[] = { 0, 0, 0 };

    if (  StarMagField::Instance() )    StarMagField::Instance()->Field( x, B );

    return TVector3( B );
  };

  virtual void     get(const double &_x, const double &_y, const double &_z, double &Bx, double &By, double &Bz ) const
  {
    double x[] = { _x, _y, _z };
    double B[] = { 0, 0, 0 };

    if (  StarMagField::Instance() ) StarMagField::Instance()->Field( x, B );

    Bx = B[0];
    By = B[1];
    Bz = B[2];
    return;
  };
};


//  Wrapper class around the forward tracker
class ForwardTracker : public ForwardTrackMaker
{
public:
  // Replaces original initialization.  Config file and hitloader
  // will be provided by the maker.
  void initialize()
  {
    LOG_INFO << "ForwardTracker::initialize()" << endm;
    nEvents = 1; // only process single event

    // Create the forward system...
    gFwdSystem = new FwdSystem( 7 );

    // make our quality plotter
    qPlotter = new QualityPlotter( cfg );
    LOG_INFO << "Booking histograms for nIterations=" << cfg.get<size_t>( "TrackFinder:nIterations", 1 ) << endm;
    qPlotter->makeHistograms( cfg.get<size_t>( "TrackFinder:nIterations", 1 ) );

    // initialize the track fitter
    trackFitter = new TrackFitter( cfg );
    trackFitter->setup( cfg.get<bool>("TrackFitter:display") );

    ForwardTrackMaker::initialize();
  }

  void finish()
  {
    LOG_SCOPE_FUNCTION( INFO);
    qPlotter->finish();
    writeEventHistograms();
  }

};


// Wrapper around the hit load.
class ForwardHitLoader : public IHitLoader
{
public:

  unsigned long long nEvents() { return 1; }
  std::map<int, std::vector<KiTrack::IHit *> > &load( unsigned long long )
  {
    return _hits;
  };

  std::map<int, std::vector<KiTrack::IHit *> > &loadSi( unsigned long long )
  {
    return _fsi_hits;
  };

  std::map<int, shared_ptr<McTrack>> &getMcTrackMap()
  {
    return _mctracks;
  };

  // Cleanup
  void clear()
  {
    _hits.clear();
    _fsi_hits.clear();
    _mctracks.clear();
  }

  // TODO, protect and add interaface for pushing hits / tracks
  std::map<int, std::vector<KiTrack::IHit *> >  _hits;
  std::map<int, std::vector<KiTrack::IHit *> >  _fsi_hits;
  std::map<int, shared_ptr<McTrack> > _mctracks;
};


StgMaker::StgMaker() : StMaker("stg"), mForwardTracker(0), mForwardHitLoader(0), mFieldAdaptor(new StarFieldAdaptor())
{
  SetAttr("useSTGC",1);                // Default sTGC on and FSI off
  SetAttr("config", "config.xml");     // Default configuration file (user may override before Init())
  SetAttr("logfile","everything.log"); // Default filename for log-guru output 
  SetAttr("fillEvent",1); // fill StEvent
};


int StgMaker::Finish()
{
  LOG_SCOPE_FUNCTION(INFO);

  mForwardTracker->finish();
  gDirectory->mkdir( "StgMaker" );
  gDirectory->cd("StgMaker");

  for (auto nh : histograms ) {
    nh.second->Write();
  }

  return kStOk;
}


int StgMaker::Init()
{
  // Initialize configuration file
  std::string configFile = SAttr("config");
  LOG_INFO << "Using tracker configuration " << configFile << endm;
  std::map<string, string> cmdLineConfig;
  jdb::XmlConfig _xmlconfig;
  _xmlconfig.loadFile( configFile, cmdLineConfig );

  // setup the loguru log file
  std::string loggerFile = SAttr("logfile");
  loguru::add_file( loggerFile.c_str(), loguru::Truncate, loguru::Verbosity_2);
  loguru::g_stderr_verbosity = loguru::Verbosity_OFF;

  mForwardTracker = new ForwardTracker();
  mForwardTracker->setConfig( _xmlconfig );

  mForwardHitLoader = new ForwardHitLoader();
  mForwardTracker->setLoader( mForwardHitLoader );
  mForwardTracker->initialize();

  // create histograms
  histograms[ "nMcTracks" ] = new TH1I( "nMcTracks", ";# MC Tracks/Event", 1000, 0, 1000 );
  histograms[ "nHitsSTGC" ] = new TH1I( "nHitsSTGC", ";# STGC Hits/Event", 1000, 0, 1000 );
  histograms[ "nHitsFSI" ] = new TH1I( "nHitsFSI", ";# FSIT Hits/Event", 1000, 0, 1000 );

  histograms[ "stgc_volume_id" ] = new TH1I( "stgc_volume_id", ";stgc_volume_id", 50, 0, 50 );
  histograms[ "fsi_volume_id" ] = new TH1I( "fsi_volume_id", ";fsi_volume_id", 50, 0, 50 );

  // there are 4 stgc stations
  for ( int i = 0; i < 4; i++ ) {
    histograms[ TString::Format("stgc%dHitMap", i).Data() ] = new TH2F( TString::Format("stgc%dHitMap", i), TString::Format("STGC Layer %d; x (cm); y(cm)"), 200, -100, 100, 200, -100, 100 );
  }

  // There are 3 silicon stations
  for ( int i = 0; i < 3; i++ ) {
    histograms[ TString::Format("fsi%dHitMap", i).Data() ] = new TH2F( TString::Format("fsi%dHitMap", i), TString::Format("FSI Layer %d; x (cm); y(cm)"), 200, -100, 100, 200, -100, 100 );
  }

  return kStOK;
};


int StgMaker::Make()
{
  LOG_INFO << "StgMaker::Make()   " << endm;

  const double z_fst[]  = { 93.3, 140.0, 186.6 };
  const double z_stgc[] = { 280.9, 303.7, 326.6, 349.4 };
  const double z_wcal[] = { 711.0 };
  const double z_hcal[] = { 740.0 }; // TODO: get correct value

  // I am a horrible person for doing this by reference, but at least
  // I don't use "goto" anywhere.
  std::map<int, shared_ptr<McTrack> > &mcTrackMap          = mForwardHitLoader->_mctracks;
  std::map<int, std::vector<KiTrack::IHit *> >  &hitMap    = mForwardHitLoader->_hits;
  std::map<int, std::vector<KiTrack::IHit *> >  &fsiHitMap = mForwardHitLoader->_fsi_hits;

  // Get geant tracks
  St_g2t_track *g2t_track = (St_g2t_track *) GetDataSet("geant/g2t_track");

  if (!g2t_track) {
    LOG_WARN << "No simulated Geant tracks found. Skipping Stg tracking..." << endm;
    return kStWarn;
  }

  LOG_INFO << "# mc tracks = " << g2t_track->GetNRows() << endm;
  histograms[ "nMcTracks" ]->Fill( g2t_track->GetNRows() );

  for ( int irow = 0; irow < g2t_track->GetNRows(); irow++ ) {
    g2t_track_st *track = (g2t_track_st *)g2t_track->At(irow);

    if (!track) continue;

    int track_id = track->id;
    float pt2 = track->p[0] * track->p[0] + track->p[1] * track->p[1];
    float pt = sqrt(pt2);
    float eta = track->eta;
    float phi = atan2(track->p[1], track->p[0]); //track->phi;
    int   q   = track->charge;

    if ( 0 == mcTrackMap[ track_id ] ) // should always happen
      mcTrackMap[ track_id ] = shared_ptr< McTrack >( new McTrack(pt, eta, phi, q) );
  }

  int count = 0;

  //
  // Use geant hits directly
  //

  /************************************************************/
  // STGC Hits
  St_g2t_fts_hit *g2t_stg_hits = (St_g2t_fts_hit *) GetDataSet("geant/g2t_stg_hit");

  int nstg = 0;

  if ( g2t_stg_hits == nullptr ) {
    LOG_INFO << "g2t_stg_hits is null" << endm;
  }
  else {
    nstg = g2t_stg_hits->GetNRows();
  }

  LOG_INFO << "# stg hits= " << nstg << endm;
  histograms[ "nHitsSTGC" ]->Fill( nstg );

  if ( IAttr("useSTGC") )
  for ( int i = 0; i < nstg; i++ ) {

    g2t_fts_hit_st *git = (g2t_fts_hit_st *)g2t_stg_hits->At(i); if (0 == git) continue; // geant hit
    int   track_id  = git->track_p;
    int   volume_id = git->volume_id;
    int   plane_id  = (volume_id - 1) / 4 ;// from 1 - 16. four chambers per station
    float x         = git->x[0] + gRandom->Gaus( 0, 0.01 );
    float y         = git->x[1] + gRandom->Gaus( 0, 0.01 );
    float z         = git->x[2] + gRandom->Gaus( 0, 0.01 );

    LOG_F( INFO, "STGC Hit: volume_id=%d, plane_id=%d, (%f, %f, %f)", volume_id, plane_id, x, y, z );
    histograms[ "stgc_volume_id" ] ->Fill( volume_id );

    if ( plane_id < 4 && plane_id >= 0 ) {
      histograms[ TString::Format("stgc%dHitMap", plane_id).Data() ] -> Fill( x, y );
    }
    else {
      LOG_F( ERROR, "Out of bounds STGC plane_id!" );
      continue;
    }

    FwdHit *hit = new FwdHit(count++, x, y, z, -plane_id, track_id, mcTrackMap[track_id] );

    // Add the hit to the hit map
    hitMap[ hit->getSector() ].push_back(hit);

    // Add hit pointer to the track
    if ( mcTrackMap[ track_id ] )   mcTrackMap[ track_id ]->addHit( hit );
  }

  /************************************************************/
  // FSI Hits
  int nfsi = 0;
  St_g2t_fts_hit *g2t_fsi_hits = (St_g2t_fts_hit *) GetDataSet("geant/g2t_fsi_hit");

  if ( g2t_fsi_hits == nullptr) {
    LOG_INFO << "g2t_fsi_hits is null" << endm;
  }
  else {
    nfsi = g2t_fsi_hits->GetNRows();
  }

  histograms[ "nHitsFSI" ]->Fill( nfsi );
  LOG_INFO << "# fsi hits = " << nfsi << endm;

  if ( IAttr("useFSI") )
  for ( int i = 0; i < nfsi; i++ ) { // yes, negative... because are skipping Si in initial tests

    g2t_fts_hit_st *git = (g2t_fts_hit_st *)g2t_fsi_hits->At(i); if (0 == git) continue; // geant hit
    int   track_id  = git->track_p;
    int   volume_id = git->volume_id; // 4, 5, 6
    int   plane_id  = volume_id - 4;
    float x         = git->x[0] + gRandom->Gaus( 0, 0.0001 );
    float y         = git->x[1] + gRandom->Gaus( 0, 0.0001 );
    float z         = git->x[2] + gRandom->Gaus( 0, 0.0001 );

    LOG_F( INFO, "FSI Hit: volume_id=%d, plane_id=%d, (%f, %f, %f)", volume_id, plane_id, x, y, z );
    histograms[ "fsi_volume_id" ] ->Fill( volume_id );

    if ( plane_id < 3 && plane_id >= 0 ) {
      histograms[ TString::Format("fsi%dHitMap", plane_id).Data() ] -> Fill( x, y );
    }
    else {
      LOG_F( ERROR, "Out of bounds FSI plane_id!" );
      continue;
    }

    FwdHit *hit = new FwdHit(count++, x, y, z, volume_id, track_id, mcTrackMap[track_id] );

    // Add the hit to the hit map
    fsiHitMap[ hit->getSector() ].push_back(hit);
  }

  LOG_INFO << "mForwardTracker -> doEvent()" << endm;

  // Process single event
  mForwardTracker -> doEvent();

  // mForwardTracker -> addSiHits();

  StEvent *stEvent = static_cast<StEvent *>(GetInputDS("StEvent"));

  if (!stEvent) {
    LOG_WARN << "No StEvent found. Stg tracks will not be saved" << endm;
    return kStWarn;
  }

  if ( IAttr("fillEvent") ) {

    // Now fill StEvent
    FillEvent();

    // Now loop over the tracks and do printout
    int nnodes = stEvent->trackNodes().size();

    for ( int i = 0; i < nnodes; i++ ) {

      const StTrackNode *node = stEvent->trackNodes()[i];
      StGlobalTrack *track = (StGlobalTrack *)node->track(global);
      StTrackGeometry *geometry = track->geometry();
      
      StThreeVectorF origin = geometry->origin();
      StThreeVectorF momentum = geometry->momentum();
      
      LOG_INFO << "-------------------------------------------------------------------------------" << endm;
      LOG_INFO << "Track # " << i << endm;
      LOG_INFO << "inner: Track origin: " << origin << " momentum: " << momentum << " pt=" << momentum.perp() << " eta=" << momentum.pseudoRapidity() << endm;
      
      StDcaGeometry *dca = track->dcaGeometry();
      if ( dca ) {
	origin = dca->origin();
	momentum = dca->momentum();
	LOG_INFO << "d c a: Track origin: " << origin << " momentum: " << momentum << " pt=" << momentum.perp() << " eta=" << momentum.pseudoRapidity() << endm;
      }
      else {
	LOG_INFO << "d c a geometry missing" << endm;
      }
      
      int idtruth = track->idTruth();
      LOG_INFO << " idtruth = " << idtruth << endm;
      auto mctrack = mcTrackMap[ idtruth ];
      
      if ( mctrack )
	LOG_INFO << "truth: pt=" << mctrack->_pt << " eta=" << mctrack->_eta << " phi=" << mctrack->_phi << " q=" << mctrack->_q << endm;
    }
    
  }

  return kStOK;
}


void StgMaker::Clear( const Option_t *opts )
{
  mForwardHitLoader->clear();
}


void StgMaker::FillEvent()
{
  StEvent *stEvent = static_cast<StEvent *>(GetInputDS("StEvent"));

  LOG_INFO << "Filling StEvent w/ results from genfit tracker" << endm;

  // Track seeds
  const auto &seed_tracks = mForwardTracker -> getRecoTracks();
  // Reconstructed globals
  const auto &glob_tracks = mForwardTracker -> globalTracks();

  // Clear up somethings... (but does this interfere w/ Sti and/or Stv?)
  StEventHelper::Remove(stEvent, "StSPtrVecTrackNode");
  StEventHelper::Remove(stEvent, "StSPtrVecPrimaryVertex");

  LOG_INFO << "  number of tracks      = " << glob_tracks.size() << endm;
  LOG_INFO << "  number of track seeds = " << seed_tracks.size() << endm;

  // StiStEventFiller fills track nodes and detector infos by reference... there
  // has got to be a cleaner way to do this, but for now follow along.
  auto &trackNodes         = stEvent->trackNodes();
  auto &trackDetectorInfos = stEvent->trackDetectorInfo();

  int track_count_total  = 0;
  int track_count_accept = 0;

  for ( auto *track : mForwardTracker->globalTracks() ) {

    // Get the track seed
    const auto &seed = seed_tracks[track_count_total];

    // Increment total track count
    track_count_total++;

    // Check to see if the track passes cuts (it should, for now)
    if ( 0 == accept(track) ) continue;

    track_count_accept++;

    // Create a detector info object to be filled
    StTrackDetectorInfo *detectorInfo = new StTrackDetectorInfo;
    FillDetectorInfo( detectorInfo, track, true );

    // Create a new track node (on which we hang a global and, maybe, primary track)
    StTrackNode *trackNode = new StTrackNode;

    // This is our global track, to be filled from the genfit::Track object "track"
    StGlobalTrack *globalTrack = new StGlobalTrack;

    // Fill the track with the good stuff
    FillTrack( globalTrack, track, seed, detectorInfo );
    FillTrackDcaGeometry( globalTrack, track );
    trackNode->addTrack( globalTrack );

    // On successful fill (and I don't see why we wouldn't be) add detector info to the list
    trackDetectorInfos.push_back( detectorInfo );

    trackNodes.push_back( trackNode );

    // // Set relationships w/ tracker object and MC truth
    // // globalTrack->setKey( key );
    // // globalTrack->setIdTruth( idtruth, qatruth ); // StTrack is dominant contributor model

    // // Add the track to its track node
    // trackNode->addTrack( globalTrack );
    // trackNodes.push_back( trackNode );

    // NOTE: could we qcall here mForwardTracker->fitTrack( seed, vertex ) ?

  } // end of loop over tracks

  LOG_INFO << "  number visited  = " <<   track_count_total  << endm;
  LOG_INFO << "  number accepted = " <<   track_count_accept << endm;
}


void StgMaker::FillTrack( StTrack *otrack, genfit::Track *itrack, const Seed_t &iseed, StTrackDetectorInfo *info )
{
  const double z_fst[]  = { 93.3, 140.0, 186.6 };
  const double z_stgc[] = { 280.9, 303.7, 326.6, 349.4 };

  // otrack == output track
  // itrack == input track (genfit)

  otrack->setEncodedMethod(kUndefinedFitterId);

  // Track length and TOF between first and last point on the track
  // TODO: is this same definition used in StEvent?
  double track_len = itrack->getTrackLen();


  //  double track_tof = itrack->getTrackTOF();
  otrack->setLength( abs(track_len) );

  // Get the so called track seed quality... the number of hits in the seed
  int seed_qual = iseed.size();
  otrack->setSeedQuality( seed_qual );

  // Set number of possible points in each detector
  // TODO: calcuate the number of possible points in each detector, for now set = 4
  otrack->setNumberOfPossiblePoints( 4, kUnknownId );


  // Calculate TheTruth from the track seed for now.   This will be fine as
  // long as we do not "refit" the track, potentially removing original seed
  // hits from the final reconstructed track.

  // Apply dominant contributor model to the track seed
  int idtruth, qatruth;
  idtruth = TheTruth( iseed, qatruth );

  otrack->setIdTruth( idtruth, qatruth ); // StTrack is dominant contributor model


  // Fill the inner and outer geometries of the track.  For now,
  // always propagate the track to the first layer of the silicon
  // to fill the inner geometry.
  //
  // TODO: We may need to extend our "geometry" classes for RK parameters
  FillTrackGeometry( otrack, itrack, z_stgc[0], kInnerGeometry );
  FillTrackGeometry( otrack, itrack, z_stgc[3], kOuterGeometry );

  // Next fill the fit traits
  FillTrackFitTraits( otrack, itrack );

  // Set detector info
  otrack->setDetectorInfo( info );

  // NOTE:  StStiEventFiller calls StuFixTopoMap here...

  // Fill the track flags
  FillTrackFlags( otrack, itrack );

  //covM[k++] = M(0,5); covM[k++] = M(1,5); covM[k++] = M(2,5); covM[k++] = M(3,5); covM[k++] = M(4,5); covM[k++] = M(5,5);
}


void StgMaker::FillTrackFlags( StTrack *otrack, genfit::Track *itrack )
{

  int flag = 0;
  // StiStEventFiller::setFlag does two things.  1) it sets the track flags, indicating
  // which detectors have participated in the track.  It is a four digit value encoded
  // as follows (from StTrack.h):

  /* --------------------------------------------------------------------------
   *  The track flag (mFlag accessed via flag() method) definitions with ITTF
   *  (flag definition in EGR era can be found at
   *   http://www.star.bnl.gov/STAR/html/all_l/html/dst_track_flags.html)
   *
   *  mFlag= zxyy, where  z = 1 for pile up track in TPC (otherwise 0)
   *                      x indicates the detectors included in the fit and
   *                     yy indicates the status of the fit.
   *  Positive mFlag values are good fits, negative values are bad fits.
   *
   *  The first digit indicates which detectors were used in the refit:
   *
   *      x=1 -> TPC only
   *      x=3 -> TPC       + primary vertex
   *      x=5 -> SVT + TPC
   *      x=6 -> SVT + TPC + primary vertex
   *      x=7 -> FTPC only
   *      x=8 -> FTPC      + primary
   *      x=9 -> TPC beam background tracks
   *
   *  The last two digits indicate the status of the refit:
   *       = +x01 -> good track
   *
   *      = -x01 -> Bad fit, outlier removal eliminated too many points
   *      = -x02 -> Bad fit, not enough points to fit
   *      = -x03 -> Bad fit, too many fit iterations
   *      = -x04 -> Bad Fit, too many outlier removal iterations
   *      = -x06 -> Bad fit, outlier could not be identified
   *      = -x10 -> Bad fit, not enough points to start
   *
   *      = +x11 -> Short track pointing to EEMC */

  // NOTE: First digit will be used as follows for forward tracks
  //
  // x = 5 sTGC only
  // x = 6 sTGC + primary vertex
  // x = 7 sTGC + forward silicon
  // x = 8 sTGC + forward silicon + primary vertex

  if      ( global  == otrack->type() ) flag = 501;
  else if ( primary == otrack->type() ) flag = 601;

  // TODO: detect presence of silicon hits and add appropriately to the flag


  // As for "bad" fits, I believe GenFit does not propagate fit information for
  // failed fits.  (???).  So we will not publish bad track flags.
  otrack->setFlag(flag);
}


void StgMaker::FillTrackMatches( StTrack *otrack, genfit::Track *itrack )
{
  // TODO:

  // At midrapidity, we extend the track to the fast detectors and check to see whether
  // the track matches an active element or not.  The fast detectors are the barrel time-
  // of-flight, the barrel EM calorimeter and teh endcap EM calorimeter.

  // We will be interested in matching FTS tracks to the following subsystems:
  // 1) The event plane detector
  // 2) Forward EM cal
  // 3) Forward Hadronic cal

  // We could adopt the following scheme to save the track fit information in a way that
  // can be accessed later, without modification to the StEvent data model...

  // Save the state of the fit (mapped to a helix) at the first silicon layer as the inner geometry.
  // Save the state of the fit (mapped to a helix) at the event plane detector as the outer geometry.
  // Save the state of the fit (mapped to a helix) at the front of the EM cal as the "Ext" geometry
  // ... helix would have no curvature at that point and would be a straight line, as there is no b field.
  // ... can easily get to the HCAL from there...
}


void StgMaker::FillTrackFitTraits( StTrack *otrack, genfit::Track *itrack )
{

  const double z_fst[]  = { 93.3, 140.0, 186.6 };
  const double z_stgc[] = { 280.9, 303.7, 326.6, 349.4 };

  unsigned short g3id_pid_hypothesis = 6; // TODO: do not hard code this

  // Set the chi2 of the fit.  The second element in the array is the incremental
  // chi2 for adding the vertex to the primary track.
  float chi2[] = {0, -999};
  const auto *fit_status = itrack->getFitStatus();

  if ( 0 == fit_status ) {
    LOG_WARN << "genfit track with no fit status" << endm;
    return;
  }

  chi2[0] = fit_status->getChi2();
  int ndf = fit_status->getNdf();

  chi2[0] /= ndf; // TODO: Check if this is right

  // ... odd that we make this determination based on the output track's type ...
  if ( primary == otrack->type() ) {
    // TODO: chi2[1] should hold the incremental chi2 of adding the vertex for the primary track
    //       is this available from genfit?
  }

  // Covariance matrix is next.  This one should be fun.  StEvent assumes the helix
  // model, but we have fit to the Runga Kutta track model.  The covariance matrix
  // is different.  So... TODO:  Do we need to specify covM for the equivalent helix?
  float covM[15] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  // Obtain fitted state so we can grab the covariance matrix
  genfit::MeasuredStateOnPlane state = itrack->getFittedState(0);

  // For global tracks, we are evaluating the fit at the first silicon plane.
  // Extrapolate the fit to this point so we can extract the covariance matrix
  // there.  For primary track, point 0 should correspond to the vertex.
  //
  // TODO: verify first point on primary tracks is the vertex.


  // ... fit traits will be evaluated at first point on track for now ...

  // if ( global == otrack->type() ) {

  //   // Obtain the cardinal representation
  //   genfit::AbsTrackRep* cardinal =  itrack->getCardinalRep();

  //   // We really don't want the overhead in the TVector3 ctor/dtor here
  //   static TVector3 xhat(1,0,0), yhat(0,1,0), Z(0,0,0);

  //   // Assign the z position
  //   Z[2] = z_fst[0];

  //   // This is the plane for which we are evaluating the fit
  //   const auto detectorPlane = genfit::SharedPlanePtr( new genfit::DetPlane(Z, xhat, yhat) );

  //   // Update the state to the given plane
  //   cardinal->extrapolateToPlane( state, detectorPlane, false, true );

  // }

  // Grab the covariance matrix
  const auto &M = state.getCov();

  // TODO: This is where we would do the math and transform from the Runga Kutta basis
  //       to the helix basis... but do we need to?

  int k = 0;
  covM[k++] = M(0, 0);
  covM[k++] = M(0, 1); covM[k++] = M(1, 1);
  covM[k++] = M(0, 2); covM[k++] = M(1, 2); covM[k++] = M(2, 2);
  covM[k++] = M(0, 3); covM[k++] = M(1, 3); covM[k++] = M(2, 3); covM[k++] = M(3, 3);
  covM[k++] = M(0, 4); covM[k++] = M(1, 4); covM[k++] = M(2, 4); covM[k++] = M(3, 4); covM[k++] = M(4, 4);

  StTrackFitTraits fit_traits(g3id_pid_hypothesis, 0, chi2, covM);

  // Get number of hits in all detectors
  int nhits[kMaxDetectorId] = {};

  for ( const auto *point : itrack->getPoints() ) {

    const auto *measurement = point->getRawMeasurement();
    int detId = measurement->getDetId();
    nhits[detId]++;

  }

  for ( int i = 0; i < kMaxDetectorId; i++ ) {
    if ( 0 == nhits[i] ) continue; // not sure why, but Sti skips setting zero hits

    fit_traits.setNumberOfFitPoints( (unsigned char)nhits[i], (StDetectorId)i );
  }

  if ( primary == otrack->type() ) {
    fit_traits.setPrimaryVertexUsedInFit( true );
  }

  otrack -> setFitTraits( fit_traits );

}


void StgMaker::FillTrackGeometry( StTrack *otrack, genfit::Track *itrack, double zplane, int io )
{
  int ipoint = 0;

  if ( io == kInnerGeometry ) ipoint = 0; // hardcoded to sTGC only for now
  else                        ipoint = 3;

  // Obtain fitted state
  genfit::MeasuredStateOnPlane measuredState = itrack->getFittedState(ipoint);

  // Obtain the cardinal representation
  genfit::AbsTrackRep *cardinal = itrack->getCardinalRep();

  // We really don't want the overhead in the TVector3 ctor/dtor here
  static TVector3 xhat(1, 0, 0), yhat(0, 1, 0), Z(0, 0, 0);

  // Assign the z position
  Z[2] = zplane;

  // This is the plane for which we are evaluating the fit
  const auto detectorPlane = genfit::SharedPlanePtr( new genfit::DetPlane(Z, xhat, yhat) );

  // Update the state to the given plane
  try {
    cardinal->extrapolateToPlane( measuredState, detectorPlane, false, true );
  }
  catch ( genfit::Exception &e ) {
    LOG_WARN << e.what() << endm;
    LOG_WARN << "Extraploation to inner/outer geometry point failed" << endm;
    return;
  }

  //  measuredState.Print();

  static StThreeVector<double> momentum;
  static StThreeVector<double> origin;

  static TVector3 pos;
  static TVector3 mom;
  static TMatrixDSym cov;

  measuredState.getPosMomCov(pos, mom, cov);

  for ( int i = 0; i < 3; i++ ) momentum[i] = mom[i];
  for ( int i = 0; i < 3; i++ ) origin[i]   = pos[i];

  double charge = measuredState.getCharge();

  // Get magnetic field
  double X[] = { pos[0], pos[1], pos[2] };
  double B[] = { 0, 0, 0 };
  StarMagField::Instance()->Field( X, B );

  // This is really an approximation, should be good enough for the inner
  // geometry (in the Silicon) but terrible in the outer geometry ( sTGC)
  double Bz = B[2];

  // Temporary helix to get the helix parameters
  StPhysicalHelix helix( momentum, origin, Bz*tesla, charge );
  // StiStEventFiller has this as |curv|.
  double curv = TMath::Abs(helix.curvature());
  double h    = -TMath::Sign(charge * Bz, 1.0); // helicity

  if ( charge == 0 ) h = 1;

  //
  // From StHelix::helix()
  //
  // phase = mPsi - h*pi/2
  // so...
  // psi = phase + h*pi/2
  //
  double psi = helix.phase() + h * TMath::Pi() / 2;
  double dip  = helix.dipAngle();
  short  q    = charge; assert( q == 1 || q == -1 || q == 0 );

  // Create the track geometry
  StTrackGeometry *geometry = new StHelixModel (q, psi, curv, dip, origin, momentum, h);

  // TODO: check helix parameters... geometry->helix() should return an StPhysicalHelix...
  //       double check that we converted everthing correctly.


  if ( kInnerGeometry == io ) otrack->setGeometry( geometry );
  else                        otrack->setOuterGeometry( geometry );
}


void StgMaker::FillTrackDcaGeometry( StGlobalTrack *otrack, genfit::Track *itrack )
{
  // We will need the event
  StEvent *stEvent = static_cast<StEvent *>(GetInputDS("StEvent"));
  assert(stEvent); // we warned ya

  // And the primary vertex
  const StPrimaryVertex* primaryVertex = stEvent->primaryVertex(0);
 
  // Obtain fitted state from genfit track
  genfit::MeasuredStateOnPlane measuredState = itrack->getFittedState(1);

  // Obtain the cardinal representation
  genfit::AbsTrackRep *cardinal =  itrack->getCardinalRep();

  static TVector3 vertex;
  double x = 0;
  double y = 0;
  double z = 0;
  if ( primaryVertex ) {
    x = primaryVertex->position()[0];
    y = primaryVertex->position()[1];
    z = primaryVertex->position()[2];
  }
  vertex.SetX(x); vertex.SetY(y); vertex.SetZ(z);

  const static TVector3 direct(0., 0., 1.); // TODO get actual beamline slope

  // Extrapolate the measured state to the DCA of the beamline
  try {
    cardinal->extrapolateToLine(  measuredState, vertex, direct, false, true );
  }
  catch ( genfit::Exception &e ) {
    LOG_WARN << e.what() << "\n"
             << "Extrapolation to beamline (DCA) failed." << "\n"
             << "... vertex " << x << " " << y << "  " << z << endm;
    return;
  }

  static StThreeVector<double> momentum;
  static StThreeVector<double> origin;

  //
  // These lines obtain the position, momentum and covariance matrix for the fit
  //

  static TVector3 pos;
  static TVector3 mom;

  measuredState.getPosMom(pos, mom);

  for ( int i = 0; i < 3; i++ ) momentum[i] = mom[i];
  for ( int i = 0; i < 3; i++ ) origin[i]   = pos[i];

  double charge = measuredState.getCharge();

  static TVectorD    state(5);
  static TMatrixDSym cov(5);

  //
  // Should be the 5D state and covariance matrix
  //  https://arxiv.org/pdf/1902.04405.pdf
  //  state = { q/p, u', v', u, v }, where
  //  q/p is charge over momentum
  //  u,  v correspond to x, y (I believe)
  //  u', v' are the direction cosines with respect to the plane
  //  ... presume that
  //      u' = cos(thetaX)
  //      v' = cos(thetaY)
  //

  state = measuredState.getState();
  cov   = measuredState.getCov();

  // Below is one way to convert the parameters to a helix, using the
  // StPhysicalHelix class

  double eta    = momentum.pseudoRapidity();
  double pt     = momentum.perp();
  double ptinv  = (pt != 0) ? 1.0 / pt : std::numeric_limits<double>::max();

  // Get magnetic field
  double X[] = { pos[0], pos[1], pos[2] };
  double B[] = { 0, 0, 0 };
  StarMagField::Instance()->Field( X, B );

  // This is really an approximation, should be good enough for the inner
  // geometry (in the Silicon) but terrible in the outer geometry ( sTGC)
  double Bz = B[2];

  // Temporary helix to get the helix parameters
  StPhysicalHelix helix( momentum, origin, Bz, charge );

  // StiStEventFiller has this as |curv|.
  double curv =  TMath::Abs(helix.curvature());
  double h    = -TMath::Sign(charge * Bz, 1.0); // helicity

  if ( charge == 0 ) h = 1;

  //
  // From StHelix::helix()
  //
  // phase = mPsi - h*pi/2
  // so...
  // psi = phase + h*pi/2
  //
  double psi  = helix.phase() + h * TMath::Pi() / 2;
  double dip  = helix.dipAngle();
  double tanl = TMath::Tan(dip); // TODO: check this
  short  q    = charge; assert( q == 1 || q == -1 || q == 0 );

  /* These are the seven parameters defined in DCA geometry...

    /// signed impact parameter; Signed in such a way that (in Sti local coords????)
    ///     x =  -impact*sin(Psi)
    ///     y =   impact*cos(Psi)
    Float_t  mImp;
    ///  Z-coordinate of this track (reference plane)
    Float_t  mZ;
    ///  Psi angle of the track
    Float_t  mPsi;
    /// signed invert pt [sign = sign(-qB)]
    Float_t  mPti;
    /// tangent of the track momentum dip angle
    Float_t  mTan;
    /// signed curvature
    Float_t  mCurv;

  */

  // TODO: is this right?
  double mImp  = origin.perp();
  double mZ    = origin[2];
  double mPsi  = psi;
  double mPti  = ptinv;
  double mTan  = tanl;
  double mCurv = curv;

  double p[] = { mImp, mZ, mPsi, mPti, mTan, mCurv };

  // TODO: fill in errors... (do this numerically?)
  double e[15] = {};

  StDcaGeometry *dca = new StDcaGeometry;
  otrack->setDcaGeometry(dca);
  dca->set(p, e);
}


void StgMaker::FillDetectorInfo( StTrackDetectorInfo *info, genfit::Track *track, bool increment )
{
  //   // here is where we would fill in
  //   // 1) total number of hits
  //   // 2) number of sTGC hits
  //   // 3) number of silicon hits
  //   // 4) an StHit for each hit fit to the track
  //   // 5) The position of the first and last hits on the track

  int ntotal   = track->getNumPoints(); // vs getNumPointsWithMeasurement() ?

  float zmin =  9E9;
  float zmax = -9E9;

  StThreeVectorF firstPoint(0, 0, 9E9);
  StThreeVectorF lastPoint(0, 0, -9E9);

  int count = 0;

  for ( const auto *point : track->getPoints() ) {

    const auto *measurement = point->getRawMeasurement();
    if ( !measurement ) {
      continue;
    }

    const TVectorD &xyz = measurement->getRawHitCoords();
    float x = xyz[0];
    float y = xyz[1];
    float z = 0; // We get this from the detector plane...

    // Get fitter info for the cardinal representation
    const auto *fitinfo = point->getFitterInfo();
    if ( !fitinfo ) {
      continue;
    }

    const auto &plane = fitinfo->getPlane();
    TVector3 normal = plane->getNormal();
    const TVector3 &origin = plane->getO();

    z = origin[2];

    if ( z > lastPoint[2] ) {
      lastPoint.setX(x);      lastPoint.setY(y);      lastPoint.setZ(z);
    }

    if ( z < firstPoint[2] ) {
      firstPoint.setX(x);     firstPoint.setY(y);     firstPoint.setZ(z);
    }

    int detId = measurement->getDetId();
    int hitId = measurement->getHitId();

    ++count;

    //TODO: Convert (or access) StHit and add to the track detector info

  }

  info->setNumberOfPoints( (unsigned char)count, kUnknownId ); // TODO assign sTGC ID


  assert(count);

  info->setFirstPoint( firstPoint );
  info->setLastPoint( lastPoint );
  info->setNumberOfPoints( ntotal, kUnknownId ); // TODO: Assign
}
