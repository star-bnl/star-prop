#ifndef VERTEX_FINDER_H
#define VERTEX_FINDER_H

#include "StgMaker/include/Tracker/FwdHit.h"

class VertexFinder
{
public:
   VertexFinder()
   {

   }

   ~VertexFinder()
   {

   }

   void findVertex( std::vector<Seed_t> tracks )
   {

      // We need a a list of points and directions vectors first build these (in a simple way for now)
      // What I want is the first point on a track and the

      for ( auto t : tracks ) {
         for ( auto hit : t ) {

         }
      }


   }



};


#endif
