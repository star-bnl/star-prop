#ifndef FWD_TRACKER_H
#define FWD_TRACKER_H

#include "TTree.h"
#include "TFile.h"
#include "TH1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TGraph2D.h"
#include "TRandom3.h"
#include "TVector3.h"
#include "Fit/Fitter.h"

#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#include "StgMaker/include/Tracker/loguru.h"
#include "StgMaker/include/Tracker/ConfigUtil.h"
#include "StgMaker/include/Tracker/FwdHit.h"
#include "StgMaker/include/Tracker/FastSim.h"
#include "StgMaker/include/Tracker/HitLoader.h"
#include "StgMaker/include/Tracker/QualityPlotter.h"
#include "StgMaker/include/Tracker/VertexFinder.h"
#include "StgMaker/include/Tracker/TrackFitter.h"

#include "KiTrack/SegmentBuilder.h"
#include "KiTrack/Automaton.h"
#include "Criteria/Criteria.h"
#include "Criteria/ICriterion.h"
#include "KiTrack/SubsetHopfieldNN.h"

#include "GenFit/FitStatus.h"

#include "StgMaker/XmlConfig/XmlConfig.h"




// Utility class for evaluating ID and QA truth
struct MCTruthUtils {

   static int domCon( std::vector<KiTrack::IHit *> hits, float &qual )
   {
      int numberOfHits = hits.size();
      std::map<int, int> count;
      int total = 0;

      for ( auto *h : hits ) {
         auto *hit = dynamic_cast<FwdHit *>( h );  if (0 == hit) continue;
         int idtruth = hit->_tid;
         count[idtruth]++; total++;
      }

      int cmx = 0; // count max
      int idtruth = 0;

      for ( auto &iter : count ) {
         if ( iter.second > cmx ) {
            cmx = iter.second;
            idtruth = iter.first;
         }
      }

      if ( total > 0 ) qual = float(cmx) / float(total);

      return idtruth;
   };


};


class ForwardTrackMaker
{
public:
   ForwardTrackMaker(  ) :
      tree(nullptr), fInput(nullptr), configFile("config.xml")
   {
      LOG_SCOPE_FUNCTION(INFO);
   }

   void setOverrides( map<string, string> cmdConfig )
   {
      cmdLineConfig = cmdConfig;
   }
   void setConfigFile( std::string cf )
   {
      configFile = cf;
   }

   // Adopt external configuration file
   void setConfig( jdb::XmlConfig &_cfg ) { cfg = _cfg; }
   // Adopt external hit loader
   void setLoader( IHitLoader *loader ) { hitLoader = loader; }

   void init()
   {
      LOG_SCOPE_FUNCTION(INFO);

      LOG_F( INFO, "CONFIG FILE: %s", configFile.c_str() );
      cfg.loadFile( configFile, cmdLineConfig );
      string datatype = cfg.get<string>( "Input:type", "sim_mc" );
      LOG_F( INFO, "Data type: %s", datatype.c_str()  );

      if ( "sim_mc" == datatype  ) {
         // initInputFromMC();
         hitLoader = new McHitLoader( cfg );
      }
      else if ( "fastsim_mc" == datatype ||  "fastsim_real" == datatype ||  "fastsim_ghost" == datatype ) {
         hitLoader = new FastSimHitLoader( cfg );
      }

      if ( nullptr != hitLoader )
         nEvents = hitLoader->nEvents();

      // Initialize the Tracker System of measure
      gFwdSystem = new FwdSystem( 7 );

      gRandom = new TRandom3();
      gRandom->SetSeed( 1 );

      // make our quality plotter
      qPlotter = new QualityPlotter( cfg );
      qPlotter->makeHistograms( cfg.get<size_t>( "TrackFinder:nIterations", 1 ) );


      trackFitter = new TrackFitter( cfg );
      trackFitter->setup( cfg.get<bool>("TrackFitter:display") );

   }


   void writeEventHistograms()
   {
      LOG_SCOPE_FUNCTION( INFO );
      // build the name
      string name = "./OUT/";
      string datatype = cfg.get<string>( "Input:type", "sim_mc" );

      if ( "sim_mc" == datatype || "fastsim_mc" == datatype ) {
         name += "MC/EventSummary_";
      }
      else if ( "fastsim_real" == datatype ) {
         name += "REAL/EventSummary_";
      }
      else if ( "fastsim_ghost" == datatype ) {
         name += "GHOST/EventSummary_";
      }

      // append STGC_ONLY if Si is not used
      if ( cfg.get<bool>("Input:si", true) == false ) {
         name += "STGC_ONLY_";
      }

      if ( cfg.get<bool>("Input:stgc", true) == false ) {
         name += "Si_ONLY_";
      }

      name += cfg.get<string>( "mod", "" );

      // add the input file as the suffix of the output filename
      name += cfg.get<string>( "Input:url" ).substr( 6 );

      if ( cfg.exists( "Output:url" ) ) {
         name = cfg.get<string>("Output:url");
      }

      LOG_F( INFO, "EventSummary : %s", name.c_str() );
      TFile *fOutput = new TFile( name.c_str(), "RECREATE" );
      fOutput->cd();
      // write out the config we use (do before histos):
      TNamed n( "cfg", cfg.toXml() );
      n.Write();

      qPlotter->writeHistograms();
      trackFitter->writeHistograms();
   }

   /** Loads Criteria from XML configuration.
    *
    * Utility function for loading criteria from XML config.
    *
    * @return vector of ICriterion pointers
    */
   std::vector<KiTrack::ICriterion *> loadCriteria( string path )
   {
      std::vector<KiTrack::ICriterion *> crits;
      auto paths = cfg.childrenOf(path);

      for ( string p : paths ) {
         string name = cfg.get<string>( p + ":name" );
         bool active = cfg.get<bool>( p + ":active", true );

         if ( false == active ) {
            LOG_F( INFO, "Skipping Criteria %s (active=false)", name.c_str() );
            continue;
         }

         float vmin = cfg.get<float>( p + ":min", 0 );
         float vmax = cfg.get<float>( p + ":max", 1 );
         LOG_F( INFO, "Loading Criteria from %s (name=%s, min=%f, max=%f)", p.c_str(), name.c_str(), vmin, vmax );
         crits.push_back(KiTrack::Criteria::createCriterion( name, vmin, vmax ));
      }

      return crits;
   }



   size_t nHitsInHitMap( std::map<int, std::vector<KiTrack::IHit *> > &hitmap )
   {
      size_t n = 0;

      for ( auto kv : hitmap ) {
         n += kv.second.size();
      }

      return n;
   }

   size_t countRecoTracks( size_t nHits )
   {
      size_t n = 0;

      for ( auto t : recoTracks ) {
         if ( t.size() == nHits )
            n++;
      }

      return n;
   }

   // this is the main event loop.  doEvent processes a single event iEvent...
   void make()
   {

      int single_event = cfg.get<int>( "Input:event", -1 );

      if ( single_event >= 0 ) {
         doEvent( single_event );
         return;
      }

      unsigned long long firstEvent = cfg.get<unsigned long long>( "Input:first-event", 0 );

      if ( cfg.exists( "Input:max-events" ) ) {
         unsigned long long maxEvents = cfg.get<unsigned long long>( "Input:max-events" );

         if ( nEvents > maxEvents )
            nEvents = maxEvents;
      }

      // loop over events
      LOG_F( INFO, "Looping on %llu events starting from event %llu", nEvents, firstEvent );

      for ( unsigned long long iEvent = firstEvent ; iEvent < firstEvent + nEvents; iEvent++ ) {
         doEvent( iEvent );
      }

      trackFitter->showEvents();
      qPlotter->finish();
      writeEventHistograms();


   }


   Seed_t::iterator findHitById( Seed_t &track, unsigned int _id )
   {
      for ( Seed_t::iterator it = track.begin(); it != track.end(); ++it ) {
         KiTrack::IHit *h = (*it);

         if ( static_cast<FwdHit *>(h)->_id == _id )
            return it;
      }

      return  track.end();
   }

   void removeHits( std::map<int, std::vector<KiTrack::IHit *> > &hitmap, std::vector<Seed_t> &tracks )
   {
      LOG_SCOPE_FUNCTION( INFO );

      for ( auto track : tracks ) {
         if ( track.size() > 0 ) {
            for ( auto hit : track ) {
               int sector = hit->getSector();

               // auto hitit = findHitById( hitmap[sector], static_cast<FwdHit*>(hit)->_id );
               auto hitit = std::find( hitmap[sector].begin(), hitmap[sector].end(), hit );

               if ( hitit ==  hitmap[sector].end() ) {
                  LOG_F( ERROR, "Hit on track but not in hitmap!" );
               }
               else {
                  hitmap[sector].erase( hitit );
                  totalHitsRemoved++;
               }

            } // loop on hits in track
         } // if track has 7 hits
      }// loop on track

      return;
   } // removeHits



   void doEvent( unsigned long long int iEvent = 0 )
   {

      // Moved cleanup to the start of doEvent, so that the fit results
      // persist after the call
      recoTracks.clear();
      fitMoms.clear();
      fitStatus.clear();

      // Clear pointers to the track reps from previous event
      for ( auto p : _globalTrackReps ) delete p;

      _globalTrackReps.clear();

      // Clear pointers to global tracks
      for ( auto p : _globalTracks ) delete p;

      _globalTracks.clear();

      LOG_SCOPE_FUNCTION(INFO);
      LOG_F( INFO, "/******************************EVENT START ********************************/" );
      LOG_F( INFO, "iEvent = %llu", iEvent );

      qPlotter->statEvent(); // starts the timer for this event

      totalHitsRemoved = 0;

      /*************************************************************/
      // Step 1
      // Load and sort the hits
      /*************************************************************/
      std::map<int, std::vector<KiTrack::IHit *> > hitmap;

      hitmap = hitLoader->load( iEvent );
      std::map<int, shared_ptr<McTrack>> &mcTrackMap = hitLoader->getMcTrackMap();


      bool mcTrackFinding = true;

      if ( cfg.exists("TrackFinder") )
         mcTrackFinding = false;

      if ( mcTrackFinding ) {
         doMcTrackFinding( mcTrackMap );
         qPlotter->summarizeEvent( recoTracks, mcTrackMap, fitMoms, fitStatus );
         //				recoTracks.clear();
         //				fitMoms.clear();
         //				fitStatus.clear();
         // trackFitter->finishEvent();
         return;
      }

      size_t nIterations = cfg.get<size_t>( "TrackFinder:nIterations", 0 );
      LOG_F( INFO, "Running %lu Tracking Iterations", nIterations );

      for ( size_t iIteration = 0; iIteration < nIterations; iIteration++ ) {
         doTrackIteration( iIteration, hitmap );
      }


      qPlotter->summarizeEvent( recoTracks, mcTrackMap, fitMoms, fitStatus );



   } // doEvent


   void doMcTrackFinding( std::map<int, shared_ptr<McTrack>> mcTrackMap)
   {

      qPlotter->startIteration();

      // we will build reco tracks from each McTrack
      for ( auto kv : mcTrackMap ) {
         auto mc_track = kv.second;

         if ( mc_track->hits.size() < 4 ) continue;

         std::set<size_t> uvid;
         Seed_t track;

         for ( auto h : mc_track->hits ) {
            track.push_back( h );
            uvid.insert( static_cast<FwdHit *>(h)->_vid );
         }

         if ( uvid.size() == track.size() ) // only add tracks that have one hit per volume
            recoTracks.push_back( track );
      }

      LOG_F( INFO, "Made %lu Reco Tracks from MC Tracks", recoTracks.size() );

      for ( auto t : recoTracks ) {
         TVector3 p = trackFitter->fitTrack( t );
         fitMoms.push_back(p);
         fitStatus.push_back( trackFitter->getStatus() );
         // Clone the track rep
         _globalTrackReps.push_back( trackFitter->getTrackRep()->clone() );
         genfit::Track *mytrack = new genfit::Track( *trackFitter->getTrack() );
         _globalTracks.push_back( mytrack );
      }

      qPlotter->afterIteration( 0, recoTracks );
   }

   void doTrackIteration( size_t iIteration, std::map<int, std::vector<KiTrack::IHit *> > &hitmap )
   {
      LOG_SCOPE_FUNCTION( INFO );
      LOG_F( INFO, "Tracking Iteration %lu", iIteration );

      // check to see if we have hits!
      size_t nHitsThisIteration = nHitsInHitMap( hitmap );

      if ( nHitsThisIteration < 4 ) {
         LOG_F( INFO, "No hits left in the hitmap! Skipping this iteration" );
         return;
      }
      else {
         LOG_F( INFO, "Working with %lu hits this iteration", nHitsThisIteration );
      }

      // this starts the timer for the iteration
      qPlotter->startIteration();

      /*************************************************************/
      // Step 2
      // build 2-hit segments (setup parent child relationships)
      /*************************************************************/
      // Initialize the segment builder with sorted hits
      KiTrack::SegmentBuilder builder( hitmap );

      // Load the criteria used for 2-hit segments
      // This loads from XML config if available
      std::string criteriaPath = "TrackFinder.Iteration[" + std::to_string(iIteration) + "].SegmentBuilder";

      if ( false == cfg.exists( criteriaPath ) ) {
         // Use the default for all iterations if it is given.
         // If not then no criteria will be applied
         criteriaPath = "TrackFinder.SegmentBuilder";
      }

      builder.addCriteria( loadCriteria( criteriaPath ) );

      // Setup the connector (this tells it how to connect hits together into segments)
      std::string connPath = "TrackFinder.Iteration[" + std::to_string(iIteration) + "].Connector";

      if ( false == cfg.exists( connPath ) ) connPath = "TrackFinder.Connector";

      unsigned int distance = cfg.get<unsigned int>( connPath + ":distance", 1 );
      LOG_F( INFO, "Connector( distance = %u )", distance );
      FwdConnector connector( distance );
      builder.addSectorConnector( &connector );

      // Get the segments and return an automaton object for further work
      LOG_F( INFO, "Getting the 1 hit segments" );
      KiTrack::Automaton automaton = builder.get1SegAutomaton();

      // at any point we can get a list of tracks out like this:
      // std::vector < std::vector< KiTrack::IHit* > > tracks = automaton.getTracks();
      // we can apply an optional parameter <nHits> to only get tracks with >=nHits in them

      // Report the number of segments and connections after the first step
      LOG_F( INFO, "nSegments=%lu", automaton.getSegments().size() );
      LOG_F( INFO, "nConnections=%u", automaton.getNumberOfConnections() );


      /*************************************************************/
      // Step 3
      // build 3-hit segments from the 2-hit segments
      /*************************************************************/
      automaton.clearCriteria();
      automaton.resetStates();
      criteriaPath = "TrackFinder.Iteration[" + std::to_string(iIteration) + "].ThreeHitSegments";

      if ( false == cfg.exists( criteriaPath ) ) criteriaPath = "TrackFinder.ThreeHitSegments";

      automaton.addCriteria( loadCriteria( criteriaPath ) );
      automaton.lengthenSegments();

      bool doAutomation = cfg.get<bool>( criteriaPath + ":doAutomation", true );
      bool doCleanBadStates = cfg.get<bool>( criteriaPath + ":cleanBadStates", true );

      if ( doAutomation ) {
         LOG_F( INFO, "Doing Automation Step" );
         automaton.doAutomaton();
      }
      else {
         LOG_F( INFO, "Not running Automation Step" );
      }

      if ( doAutomation && doCleanBadStates ) {
         automaton.cleanBadStates();
      }

      LOG_F( INFO, "nSegments=%lu", automaton.getSegments().size() );
      LOG_F( INFO, "nConnections=%u", automaton.getNumberOfConnections() );

      /*************************************************************/
      // Step 4
      // Get the tracks from the possible tracks that are the best subset
      /*************************************************************/
      std::string subsetPath = "TrackFinder.Iteration[" + std::to_string(iIteration) + "].SubsetNN";

      if ( false == cfg.exists( subsetPath ) ) subsetPath = "TrackFinder.SubsetNN";

      //  only for debug really
      bool findSubsets = cfg.get<bool>( subsetPath + ":active", true );

      if ( findSubsets ) {
         LOG_SCOPE_F( INFO, "SubsetNN" );
         LOG_F( INFO, "Trying to get best set of tracks given all the possibilities" );

         size_t minHitsOnTrack = cfg.get<size_t>( subsetPath + ":min-hits-on-track", 7 );
         LOG_F( INFO, "Getting all tracks with at least %lu hits on them", minHitsOnTrack );
         std::vector<Seed_t> tracks = automaton.getTracks(minHitsOnTrack);
         LOG_F( INFO, "We have %lu Tracks to work with", tracks.size() );

         float omega = cfg.get<float>( subsetPath + ".Omega", 0.75 );
         float stableThreshold = cfg.get<float>( subsetPath + ".StableThreshold", 0.1 );
         float Ti = cfg.get<float>( subsetPath + ".InitialTemp", 2.1 );
         float Tf = cfg.get<float>( subsetPath + ".InfTemp", 0.1 );

         LOG_F( INFO, "SubsetHopfiledNN Settings:" );
         LOG_F( INFO, "Temp (initial=%0.3f, inf=%0.3f)", Ti, Tf );
         LOG_F( INFO, "Omega=%0.3f", omega );
         LOG_F( INFO, "StableThreshold=%0.3f", stableThreshold );

         KiTrack::SubsetHopfieldNN<Seed_t> subset;
         subset.add( tracks );
         subset.setOmega( omega );
         subset.setLimitForStable( stableThreshold );
         subset.setTStart( Ti );

         SeedCompare comparer;
         SeedQual quality;

         subset.calculateBestSet( comparer, quality );

         std::vector<Seed_t> acceptedTracks = subset.getAccepted();
         std::vector<Seed_t> rejectedTracks = subset.getRejected();

         LOG_F( INFO, "We had %lu tracks. Accepted = %lu, Rejected = %lu", tracks.size(), acceptedTracks.size(), rejectedTracks.size() );

         // Add the set of accepted tracks to our collection of found tracks from all iterations
         recoTracks.insert( recoTracks.end(), acceptedTracks.begin(), acceptedTracks.end() );


         for ( auto t : acceptedTracks ) {
            TVector3 p = trackFitter->fitTrack( t );
            fitMoms.push_back(p);
            fitStatus.push_back( trackFitter->getStatus() );
            // Clone the track rep
            _globalTrackReps.push_back( trackFitter->getTrackRep()->clone() );
            genfit::Track *mytrack = new genfit::Track( *trackFitter->getTrack() );
            float qatruth;
            int idtruth = MCTruthUtils::domCon( t, qatruth );
            mytrack->setMcTrackId( idtruth );
            _globalTracks.push_back( mytrack );

         }

         qPlotter->afterIteration( iIteration, acceptedTracks );
         // checkFoundTracks( hits_map_by_track, acceptedTracks, "2" );

         // Find vertex
         // VertexFinder vf;
         // vf.findVertex( recoTracks );



         /*************************************************************/
         // Step 5
         // Remove the hits from any track that was found
         /*************************************************************/
         std::string hrmPath = "TrackFinder.Iteration[" + std::to_string(iIteration) + "].SubsetNN";

         if ( false == cfg.exists( hrmPath ) ) hrmPath = "TrackFinder.SubsetNN";

         if ( true == cfg.get<bool>( hrmPath + ":active", true ) ) {
            LOG_F( INFO, "Removing hits, BEFORE n = %lu", nHitsInHitMap( hitmap ) );
            removeHits( hitmap, acceptedTracks );
            LOG_F( INFO, "Removing hits, AFTER n = %lu", nHitsInHitMap( hitmap ) );
         }
         else {
            LOG_F( INFO, "Hit Remover is turned OFF" );
         }
      }
      else {  // the subset and hit removal
         LOG_F( INFO, "The SubsetNN Step is turned OFF. This also means the Hit Remover is turned OFF (requires SubsetNN step)" );

         size_t minHitsOnTrack = cfg.get<size_t>( subsetPath + ":min-hits-on-track", 7 );
         LOG_F( INFO, "Getting all tracks with at least %lu hits on them", minHitsOnTrack );
         std::vector<Seed_t> tracks = automaton.getTracks(minHitsOnTrack);
         LOG_F( INFO, "We have %lu Tracks to work with", tracks.size() );

         for ( auto t : tracks ) {
            TVector3 p = trackFitter->fitTrack( t );
            fitMoms.push_back(p);
         }

         qPlotter->afterIteration( iIteration, tracks );



      }
   } // doTrackIteration


protected:
   FastSim *fastSim;
   TTree *tree;
   TFile *fInput;
   unsigned long long int nEvents;

   /* TTree data members */
   int tree_n;
   const static unsigned int tree_max_n = 5000;
   int tree_vid[tree_max_n], tree_tid[tree_max_n];
   float tree_x[tree_max_n], tree_y[tree_max_n], tree_z[tree_max_n], tree_pt[tree_max_n];


   jdb::XmlConfig cfg;
   map<string, string> cmdLineConfig;
   std::string configFile;
   // event level summary histograms
   map<string, TH1 *> histograms;

   vector<size_t> trueTracksWithNHits;
   float total7HitTrue;
   float total7HitFound;
   size_t totalHitsRemoved;
   size_t nTrueTracks;
   size_t nTrueTracksWith7;

   std::vector<Seed_t> recoTracks; // the tracks recod from all iterations
   std::vector<TVector3> fitMoms;
   // vector<int> fitQs;
   std::vector<genfit::FitStatus>     fitStatus;
   std::vector<genfit::AbsTrackRep *> _globalTrackReps;
   std::vector<genfit::Track *>       _globalTracks;

   QualityPlotter *qPlotter;
   IHitLoader *hitLoader;

   TrackFitter *trackFitter = nullptr;

public:
   const std::vector<Seed_t> &getRecoTracks() const { return recoTracks; }
   const std::vector<TVector3> &getFitMomenta() const { return fitMoms; }
   const std::vector<genfit::FitStatus> &getFitStatus() const { return fitStatus; }
   const std::vector<genfit::AbsTrackRep *> &globalTrackReps() const { return _globalTrackReps; }
   const std::vector<genfit::Track *> &globalTracks() const { return _globalTracks; }

};



#endif
