#ifndef HIT_LOADER_H
#define HIT_LOADER_H

#include <map>

#include "TFile.h"
#include "TTree.h"
#include "TRandom.h"
#include "TRandom3.h"

#include "StgMaker/include/Tracker/FwdHit.h"
#include "StgMaker/include/Tracker/loguru.h"
#include "StgMaker/include/Tracker/ConfigUtil.h"

#include "StgMaker/XmlConfig/XmlConfig.h"

class IHitLoader
{
public:

   virtual unsigned long long nEvents() = 0;
   virtual std::map<int, std::vector<KiTrack::IHit *> > &load( unsigned long long iEvent ) = 0;
   virtual std::map<int, shared_ptr<McTrack>> &getMcTrackMap() = 0;

};

class McHitLoader : public IHitLoader
{

public:
   McHitLoader(  jdb::XmlConfig &_cfg ) : cfg(_cfg)
   {
      LOG_F( INFO, "Loading data from : %s", cfg.get<TString>( "Input:url", "../../SimHitsTuple.root").Data() );
      fInput = new TFile( cfg.get<TString>( "Input:url", "../../SimHitsTuple.root") );
      tree = (TTree *)fInput->Get("hits" );

      // nEvents = tree->GetEntries();
      LOG_F( INFO, "Tree has %lld entries", tree->GetEntries() );

      string datatype = cfg.get<string>( "Input:type", "sim_mc" );

      // load the MC level hits (no ghosts and no postion blurring)
      if ( "sim_mc" == datatype ) {
         tree->SetBranchAddress("n", &tree_n);
         tree->SetBranchAddress("x", &tree_x);
         tree->SetBranchAddress("y", &tree_y);
         tree->SetBranchAddress("z", &tree_z);
         tree->SetBranchAddress("vid", &tree_vid);
         tree->SetBranchAddress("tid", &tree_tid);

         if ( tree->GetBranch( "pt" ) != nullptr ) {
            tree->SetBranchAddress( "pt", &tree_pt );
         }
      }
   } // ctor

   unsigned long long nEvents()
   {
      if ( nullptr != tree )
         return tree->GetEntries();

      return 0;
   }
   std::map<int, std::vector<KiTrack::IHit *> > &load( unsigned long long iEvent )
   {
      LOG_SCOPE_FUNCTION( INFO );

      tree->GetEntry( iEvent );

      // std::map<int, std::vector<IHit*> > hitmap; // key is sector, value is vector of hits

      // TODO we need to clear a lot of pointers
      hitmap.clear();
      mcTrackMap.clear();
      hits_map_by_track.clear();

      if ( tree == nullptr )
         return hitmap;

      // TODO: build a reverse map key = hit id, value = hit or iterator in hitmap (for removing the hits later on and avoid the search over vector of hits)
      LOG_F( INFO, "Loading %d hits into hitmap", tree_n );

      for ( unsigned int i = 0; i < tree_n; i++ ) {
         int vid = tree_vid[i]; // the volume id for this hit
         int tid = tree_tid[i]; // the track_p
         float x = tree_x[i];
         float y = tree_y[i];
         float z = tree_z[i];



         LOG_F( 3, "Hit at (%0.2f, %0.2f, %0.2f) in vol=%d from track=%d", x, y, z, vid, tid );

         // build the McTrack Object if we are the first one
         if ( mcTrackMap.count( tid ) == 0) {
            mcTrackMap[ tid ] = shared_ptr< McTrack >( new McTrack( tree_pt[i] ) );
         }

         // Create the Hit
         FwdHit *fhit = new FwdHit( i, x, y, z, vid, tid, mcTrackMap[ tid ] );

         // fill hitmap
         hitmap[ fhit->getSector() ].push_back( fhit ); // this hitmap is actually used
         hits_map_by_track[ tree_tid[i] ].push_back( fhit ); // this one is for evaluation
         // add pointer to track map
         mcTrackMap[ tid ]->addHit( fhit );
      } // loop on tree_n

      return hitmap;
   }

   virtual std::map<int, shared_ptr<McTrack>> &getMcTrackMap()
   {
      return mcTrackMap;
   }

protected:
   jdb::XmlConfig &cfg;
   TFile *fInput;
   TTree *tree;


   // Data
   std::map<int, std::vector<KiTrack::IHit *> > hitmap;
   std::map<int, shared_ptr<McTrack>> mcTrackMap; // key is track id (track_p) value is McTrack
   std::map< int, std::vector<KiTrack::IHit *> > hits_map_by_track; // this is used for evaluation with MC tracks

   /* TTree data members */
   int tree_n;
   const static unsigned int tree_max_n = 5000;
   int tree_vid[tree_max_n], tree_tid[tree_max_n];
   float tree_x[tree_max_n], tree_y[tree_max_n], tree_z[tree_max_n], tree_pt[tree_max_n];
};


class FastSimHitLoader : public IHitLoader
{
public:
   FastSimHitLoader(  jdb::XmlConfig &_cfg ) : cfg(_cfg)
   {

      fInput = new TFile( cfg.get<TString>( "Input:url", "../../SimHitsTuple.root") );
      tree = (TTree *)fInput->Get("hits" );

      // nEvents = tree->GetEntries();
      LOG_F( INFO, "Tree has %lld entries", tree->GetEntries() );

      datatype = cfg.get<string>( "Input:type", "fastsim_mc" );
      useSi = cfg.get<bool>( "Input:si", true );
      useStgc = cfg.get<bool>( "Input:stgc", true );

      sigmaMcX = cfg.get<float>( "Input:blurX", 0 );
      sigmaMcY = cfg.get<float>( "Input:blurY", 0 );

      sigmaMcR = cfg.get<float>( "Input:blurR", 0 );
      sigmaMcPhi = cfg.get<float>( "Input:blurPhi", 0 );
      rand = new TRandom3();
      size_t rSeed = cfg.get<size_t>( "Input:seed", 0 );
      LOG_F( INFO, "random seed : %lu", rSeed );
      rand->SetSeed( rSeed );
      LOG_F( INFO, "Blur MC points, sigmaMcX=%f, sigmaMcY=%f", sigmaMcX, sigmaMcY );

      nNoise = cfg.get<size_t>( "Input:nNoise", 0 );
      LOG_F( INFO, "Adding %lu Noise hits", nNoise );

      tree->SetBranchAddress( "mchn", &mchn);
      tree->SetBranchAddress( "mchx", &mchx);
      tree->SetBranchAddress( "mchy", &mchy);
      tree->SetBranchAddress( "mchz", &mchz);
      tree->SetBranchAddress( "mchvid", &mchvid);
      tree->SetBranchAddress( "mchtid", &mchtid);

      tree->SetBranchAddress( "mctn", &mctn);
      tree->SetBranchAddress( "mctpt", &mctpt);
      tree->SetBranchAddress( "mcteta", &mcteta);
      tree->SetBranchAddress( "mctphi", &mctphi);
      tree->SetBranchAddress( "mctq", &mctq);


      tree->SetBranchAddress( "ftsn", &ftsn);
      tree->SetBranchAddress( "ftsx", &ftsx);
      tree->SetBranchAddress( "ftsy", &ftsy);
      tree->SetBranchAddress( "ftsmcx1", &ftsmcx1);
      tree->SetBranchAddress( "ftsmcy1", &ftsmcy1);
      tree->SetBranchAddress( "ftsmcx2", &ftsmcx2);
      tree->SetBranchAddress( "ftsmcy2", &ftsmcy2);
      tree->SetBranchAddress( "ftsz", &ftsz);
      tree->SetBranchAddress( "ftsvid", &ftsvid);
      tree->SetBranchAddress( "ftstid", &ftstid);
   }
   unsigned long long nEvents()
   {
      if ( nullptr != tree )
         return tree->GetEntries();

      return 0;
   }
   std::map<int, std::vector<KiTrack::IHit *> > &load( unsigned long long iEvent )
   {
      LOG_SCOPE_FUNCTION( INFO );
      tree->GetEntry( iEvent ); // get this events entries

      // TODO we need to clear a lot of pointers
      hitmap.clear();
      mcTrackMap.clear();
      hits_map_by_track.clear();

      if ( tree == nullptr )
         return hitmap;

      // build the mc track map first
      for ( size_t i = 0; i < mctn; i++ ) {
         size_t tid = i + 1;
         mcTrackMap[ tid ] = shared_ptr< McTrack >( new McTrack( mctpt[i], mcteta[i], mctphi[i], mctq[i] ) );
      }

      if ( "fastsim_mc" == datatype ) {
         return loadMC();
      }
      else if ( "fastsim_real" == datatype ) {
         return loadFastSim( false );
      }
      else if ( "fastsim_ghost" == datatype ) {
         return loadFastSim( true );
      }

      return hitmap;
   }


   std::map<int, std::vector<KiTrack::IHit *> > &loadMC( )
   {
      LOG_SCOPE_FUNCTION( INFO );
      // TODO: build a reverse map key = hit id, value = hit or iterator in hitmap (for removing the hits later on and avoid the search over vector of hits)
      LOG_F( INFO, "Loading %d hits into hitmap", mchn );

      for ( unsigned int i = 0; i < mchn; i++ ) {
         int vid = mchvid[i]; // the volume id for this hit
         int tid = mchtid[i]; // the track_p
         float x = mchx[i];
         float y = mchy[i];
         float z = mchz[i];

         if ( sigmaMcX > 0.0001 ) // 1 micron
            x += rand->Gaus( 0, sigmaMcX );

         if ( sigmaMcY > 0.0001 ) // 1 micron
            y += rand->Gaus( 0, sigmaMcY );

         // if ( sigmaMcR > 0.0001 ){ // 1 micron
         // 	x += rand->Gaus( 0, sigmaMcX );
         // }
         // if ( sigmaMcY > 0.0001 ) // 1 micron
         // 	y += rand->Gaus( 0, sigmaMcY );
         LOG_F( INFO, "Hit at (%0.2f, %0.2f, %0.2f) in vol=%d from track=%d", x, y, z, vid, tid );

         if ( false == useSi ) {
            if ( vid < 9 ) continue;

            vid = (vid - 9) * -1; // negative means use directly
         }

         // Create the Hit
         FwdHit *fhit = new FwdHit( i, x, y, z, vid, tid, mcTrackMap[ tid ] );

         // fill hitmap
         hitmap[ fhit->getSector() ].push_back( fhit ); // this hitmap is actually used

         // hits_map_by_track[ tree_tid[i] ].push_back( fhit ); // this one is for evaluation

         // add pointer to track map
         mcTrackMap[ tid ]->addHit( fhit );

      } // loop on tree_n

      for ( unsigned int i = 0; i < nNoise; i++ ) {



         int ti1 = rand->Integer( mchn );
         int ti2 = rand->Integer( mchn );

         int tid = 0; // the track_p
         float x = mchx[ti1];
         float y = mchy[ti2];
         float z = mchz[ti2];

         int vid = mchvid[ti1]; // the volume id for this hit

         if ( false == useSi ) {
            if ( vid < 9 ) continue;

            vid = (vid - 9) * -1; // negative means use directly
         }

         if ( sigmaMcX > 0.0001 ) // 1 micron
            x += rand->Gaus( 0, sigmaMcX );

         if ( sigmaMcY > 0.0001 ) // 1 micron
            y += rand->Gaus( 0, sigmaMcY );

         LOG_F( INFO, "Hit at (%0.2f, %0.2f, %0.2f) in vol=%d from track=%d", x, y, z, vid, tid );



         // Create the Hit
         FwdHit *fhit = new FwdHit( i, x, y, z, vid, tid, nullptr );
         // fill hitmap
         hitmap[ fhit->getSector() ].push_back( fhit ); // this hitmap is actually used
      }

      return hitmap;
   }

   std::map<int, std::vector<KiTrack::IHit *> > &loadFastSim( bool includeGhosts = false )
   {
      LOG_SCOPE_FUNCTION( INFO );
      // TODO: build a reverse map key = hit id, value = hit or iterator in hitmap (for removing the hits later on and avoid the search over vector of hits)
      LOG_F( INFO, "Loading %d hits into hitmap", ftsn );

      for ( unsigned int i = 0; i < ftsn; i++ ) {
         int vid = ftsvid[i]; // the volume id for this hit
         int tid = ftstid[i]; // the track_p
         float x = ftsx[i];
         float y = ftsy[i];
         float z = ftsz[i];
         LOG_F( 3, "Hit at (%0.2f, %0.2f, %0.2f) in vol=%d from track=%d", x, y, z, vid, tid );

         // if ( vid < 7 ) continue;
         // if ( x < 0 || y < 0 ) continue;
         // if ( vid == 10 ) continue;

         if ( false == useSi ) {
            if ( vid < 9 ) continue;

            vid = (vid - 9) * -1; // negative means use directly
         }

         if ( false == useStgc ) {
            if (vid > 8) continue;
         }

         if ( includeGhosts == false && tid == 0 ) continue;

         // Create the Hit
         FwdHit *fhit = new FwdHit( i, x, y, z, vid, tid, mcTrackMap[ tid ] );

         // fill hitmap
         hitmap[ fhit->getSector() ].push_back( fhit ); // this hitmap is actually used

         // hits_map_by_track[ tree_tid[i] ].push_back( fhit ); // this one is for evaluation

         // add pointer to track map
         if ( mcTrackMap.count( tid ) && mcTrackMap[ tid ] != nullptr )
            mcTrackMap[ tid ]->addHit( fhit );

      } // loop on tree_n

      return hitmap;
   }

   virtual std::map<int, shared_ptr<McTrack>> &getMcTrackMap()
   {
      return mcTrackMap;
   }

protected:
   jdb::XmlConfig &cfg;
   TFile *fInput;
   TTree *tree;
   string datatype;
   bool useSi = false;
   bool useStgc = false;

   float sigmaMcX = 0;
   float sigmaMcY = 0;

   float sigmaMcR = 0;
   float sigmaMcPhi = 0;
   TRandom *rand;

   size_t nNoise = 0;

   // Data
   std::map<int, std::vector<KiTrack::IHit *> > hitmap;
   std::map<int, shared_ptr<McTrack>> mcTrackMap; // key is track id (track_p) value is McTrack
   std::map< int, std::vector<KiTrack::IHit *> > hits_map_by_track; // this is used for evaluation with MC tracks


   const static unsigned int tree_max_n = 9999;
   const static unsigned int tree_max_track = 4999;
   // tree data
   unsigned int mchn, mchvid[tree_max_n], mchtid[tree_max_n];
   float mchx[tree_max_n], mchy[tree_max_n], mchz[tree_max_n];

   unsigned int mctn;
   float mctpt[tree_max_track], mcteta[tree_max_track], mctphi[tree_max_track];
   int mctq[tree_max_track];


   // FTS hits
   unsigned int ftsn, ftsvid[tree_max_n], ftstid[tree_max_n];
   float ftsx[tree_max_n], ftsy[tree_max_n], ftsz[tree_max_n];
   float ftsmcx1[tree_max_n], ftsmcy1[tree_max_n], ftsmcx2[tree_max_n], ftsmcy2[tree_max_n];

};



#endif
