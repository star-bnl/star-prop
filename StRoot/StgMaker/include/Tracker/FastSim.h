#ifndef FAST_SIM_H
#define FAST_SIM_H

#include "StgMaker/include/Tracker/loguru.h"
#include "StgMaker/include/Tracker/FwdHit.h"

#include <map>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TRandom3.h"

class FastSim
{
public:
   FastSim( ULong_t seed = 1 )
   {
      gRandom = new TRandom3();
      gRandom->SetSeed( seed );
   }

   void load( std::string name )
   {
      LOG_SCOPE_FUNCTION(INFO);
      TFile *fInput = new TFile( name.c_str() );
      TTree *tree = (TTree *)fInput->Get("hits" );

      size_t nEvents = tree->GetEntries();
      LOG_F( INFO, "Tree has %lld entries", tree->GetEntries() );

      tree->SetBranchAddress("n", &tree_n);
      tree->SetBranchAddress("x", &tree_x);
      tree->SetBranchAddress("y", &tree_y);
      tree->SetBranchAddress("z", &tree_z);
      tree->SetBranchAddress("vid", &tree_vid);
      tree->SetBranchAddress("tid", &tree_tid);

      unsigned int iGlobal = 0;

      for ( size_t i = 0 ; i < nEvents; i++ ) {
         tree->GetEntry( i );
         std::map< int, std::vector<KiTrack::IHit *> > hits_map_by_track;

         for ( unsigned int i = 0; i < tree_n; i++ ) {
            int vid = tree_vid[i]; // the volume id for this hit
            float x = tree_x[i] + gRandom->Gaus( 0.0, 0.01 );
            float y = tree_y[i] + gRandom->Gaus( 0.0, 0.01 );
            float z = tree_z[i];
            LOG_F( 3, "Hit at (%0.2f, %0.2f, %0.2f) in vol=%d from track=%d", x, y, z, vid, tree_tid[i] );
            FwdHit *fhit = new FwdHit( iGlobal, x, y, tree_z[i], tree_vid[i], tree_tid[i] );
            hits_map_by_track[ tree_tid[i] ].push_back( fhit ); // this one is for evaluation
            iGlobal++;
         }

         for ( auto kv : hits_map_by_track ) {
            if ( kv.second.size() == 7 )
               buffer.push_back( kv.second );
         }

      } // loop on events

      LOG_F( INFO, "Finished loading buffer" );

   }// load

   std::map<int, std::vector<KiTrack::IHit *> > genEvent( size_t nTracks = 1 )
   {
      LOG_SCOPE_FUNCTION(INFO);
      std::map<int, std::vector<KiTrack::IHit *> > hitmap;


      for ( size_t iTrack = 0; iTrack < nTracks; iTrack++ ) {
         size_t iiTrack = gRandom->Integer( buffer.size() - 1 );

         for ( KiTrack::IHit *hit : buffer[iiTrack] ) {
            hitmap[ hit->getSector() ].push_back( hit );
         }
      }// loop on nTracks


      return hitmap;
   }// genEvent

   std::vector< std::vector<KiTrack::IHit *> > buffer;
   int tree_n;
   const static unsigned int tree_max_n = 500;
   int tree_vid[tree_max_n], tree_tid[tree_max_n];
   float tree_x[tree_max_n], tree_y[tree_max_n], tree_z[tree_max_n];
};


#endif
