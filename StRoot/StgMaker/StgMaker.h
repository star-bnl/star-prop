#ifndef __StgMaker_h__
#define __StgMaker_h__

#include "StMaker.h"

#ifndef __CINT__
#include "StgUtil/XmlConfig/XmlConfig.h"
#include "Track.h"

namespace KiTrack { 
  class IHit;
};

#endif

class ForwardTracker;
class ForwardHitLoader;
class StarFieldAdaptor;

class StRnDHitCollection;
class StTrack;
class StTrackDetectorInfo;

#include <vector>

class StgMaker : public StMaker {

  ClassDef(StgMaker,0);

#ifndef __CINT__
  XmlConfig _xmlconfig;
#endif

public:

  StgMaker();
  ~StgMaker(){ /* nada */ };

  int  Init();
  int  Make();
  void Clear( const Option_t* opts="" );

  enum { kInnerGeometry,       kOuterGeometry };

private:
protected:

#ifndef __CINT__
  ForwardTracker*        mForwardTracker;
  ForwardHitLoader*      mForwardHitLoader;
  StarFieldAdaptor*      mFieldAdaptor;

  using Seed_t = std::vector<KiTrack::IHit*>;

  // Fill StEvent
  void FillEvent();
  void FillDetectorInfo  ( StTrackDetectorInfo* info,   genfit::Track* track, bool increment );
  void FillTrack         ( StTrack*             otrack, genfit::Track* itrack, const Seed_t& iseed, StTrackDetectorInfo* info );
  void FillTrackFlags    ( StTrack*             otrack, genfit::Track* itrack );
  void FillTrackGeometry ( StTrack*             otrack, genfit::Track* itrack, double zplane, int io );
  void FillTrackDcaGeometry ( StTrack*             otrack, genfit::Track* itrack );
  void FillTrackFitTraits( StTrack*             otrack, genfit::Track* itrack );
  void FillTrackMatches  ( StTrack*             otrack, genfit::Track* itrack );


#endif
  
};

#endif
