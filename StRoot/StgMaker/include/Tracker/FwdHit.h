#ifndef FWD_HIT_H
#define FWD_HIT_H

#include "KiTrack/IHit.h"
#include "KiTrack/ISectorConnector.h"
#include "KiTrack/ISectorSystem.h"
#include "KiTrack/KiTrackExceptions.h"

#include <string.h>
#include <vector>
#include <set>
#include <memory>

class StHit;



class FwdSystem : public KiTrack::ISectorSystem
{
public:
   FwdSystem( const int ndisks ) : KiTrack::ISectorSystem(), _ndisks(ndisks)   {  };
   ~FwdSystem() { /* */ };
   virtual unsigned int getLayer( int diskid ) const throw ( KiTrack::OutOfRange )
   {
      // return diskid;
      // Have to remap disk number to layers 1-7
      //          1,2,3,4,5,6, 7,8, 9,10, 11, 12
      // int _map[]={0,0,0,1,2,3, 0,0, 4, 5,  6, 7}; // ftsref6a
      // return _map[diskid-1];
      return diskid;
   }

   // Decoder ring
   // int decode(int volid) const { return volid; } // TODO: make dependence on geometry
   int _ndisks;
   std::string getInfoOnSector( int sec ) const { return "TODO"; }
};
FwdSystem *gFwdSystem;
//_____________________________________________________________________________________________


// small class to store Mc Track information
class McTrack
{
public:
   McTrack()
   {
      _pt = -999;
      _eta = -999;
      _phi = -999;
      _q = 0;
   }
   McTrack( float pt, float eta = -999, float phi = -999, int q = 0 )
   {
      _pt = pt;
      _eta = eta;
      _phi = phi;
      _q = q;
   }
   void addHit( KiTrack::IHit *hit )
   {
      hits.push_back( hit );
   }

   float _pt, _eta, _phi;
   int _tid, _q;

   std::vector<KiTrack::IHit *> hits;
};

class FwdHit : public KiTrack::IHit
{
public:
   FwdHit(unsigned int id, float x, float y, float z, int vid, int tid, std::shared_ptr<McTrack> mcTrack = nullptr ) : KiTrack::IHit()
   {
      _id = id;
      _x = x;
      _y = y;
      _z = z;
      _tid = tid;
      _vid = vid;
      _mcTrack = mcTrack;
      _hit = 0;

      int _map[] = {0, 0, 0, 0, 0, 1, 2, 0, 0, 3, 4,  5, 6}; // ftsref6a

      if ( vid > 0 )
         _sector = _map[vid];
      else {
         _sector = abs(vid); // set directly if you want
         // now set vid back so we retain info on the tru origin of the hit
         _vid = abs(vid) + 9; // we only use this for sTGC only.  Needs to be cleaner in future.
      }

      //gFwdSystem->getLayer(diskid);


      // Need also to set _3DAngle, _phiMV and _thetaMV (and understand their definitions...)
      // ... looks like it is detector specific...
   };

   const KiTrack::ISectorSystem *getSectorSystem() const { return gFwdSystem; } // need to implement
   // StHit* _hit;

   int _tid; // aka ID truth
   int _vid;
   unsigned int _id; // just a unique id for each hit in this event.
   std::shared_ptr<McTrack> _mcTrack;

   StHit *_hit;

};

using Seed_t = std::vector<KiTrack::IHit *>;

class FwdConnector : public KiTrack::ISectorConnector
{
public:
   FwdConnector( unsigned int distance ) : _system(*gFwdSystem), _distance(distance) { }
   ~FwdConnector() { /**/ };

   // Return the possible sectors (layers) given current
   virtual std::set<int> getTargetSectors( int disk )
   {

      //                  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12
      int _targets[] = {  0,  0,  0,  0,  4,  5,  0,  0,  6,  9, 10, 11 };
      std::set<int> r;

      if (disk > 0 && _distance >= 1)
         r.insert( disk - 1 );

      if (disk > 1 && _distance >= 2)
         r.insert( disk - 2 );

      if (disk > 2 && _distance >= 3)
         r.insert( disk - 3 );

      if (disk > 3 && _distance >= 4)
         r.insert( disk - 4 );

      return r;

      // if ( disk >= 1 )
      // 	r.insert( _targets[disk-1] );
      // else {
      // 	LOG_F( ERROR, "disk should be >= 1" );
      // }
      // return r;
   };

private:
protected:

   const FwdSystem _system;   // numbering system
   unsigned int  _distance; // number of layers forward to search
}; // FwdConnector


class SeedQual
{
public:
   inline double operator()( Seed_t s ) { return double(s.size()) / 7.0; }
};

class SeedCompare
{
public:
   inline bool operator()( Seed_t trackA, Seed_t trackB )
   {
      std::map< unsigned int, unsigned int > hit_counts;
      // we are assuming that the same hit can never be used twice on a single track!

      for ( auto h : trackA ) {
         hit_counts[ static_cast<FwdHit *>(h)->_id ]++;
      }

      // now look at the other track and see if it has the same hits in it
      for ( auto h : trackB ) {
         hit_counts[ static_cast<FwdHit *>(h)->_id ]++;

         // incompatible if they share a single hit
         if ( hit_counts[ static_cast<FwdHit *>(h)->_id ] >= 2 ) {
            return false;
         }
      }

      // no hits are shared, they are compatible
      return true;
   }
};




#endif
