#ifndef StgMaker_h
#define StgMaker_h

#include "StChain/StMaker.h"

#include "GenFit/Track.h"

namespace KiTrack
{
class IHit;
};


class ForwardTracker;
class ForwardHitLoader;
class StarFieldAdaptor;

class StRnDHitCollection;
class StTrack;
class StTrackDetectorInfo;

#include <vector>

class StgMaker : public StMaker
{
public:

  StgMaker();
  ~StgMaker() { /* nada */ };

  int  Init();
  int Finish();
  int  Make();
  void Clear( const Option_t *opts = "" );

  enum { kInnerGeometry,       kOuterGeometry };

private:

  ForwardTracker        *mForwardTracker;
  ForwardHitLoader      *mForwardHitLoader;
  StarFieldAdaptor      *mFieldAdaptor;

  typedef std::vector<KiTrack::IHit *> Seed_t;

  std::map< std::string, TH1 * > histograms;

  // Fill StEvent
  void FillEvent();
  void FillDetectorInfo  ( StTrackDetectorInfo *info,   genfit::Track *track, bool increment );
  void FillTrack         ( StTrack             *otrack, genfit::Track *itrack, const Seed_t &iseed, StTrackDetectorInfo *info );
  void FillTrackFlags    ( StTrack             *otrack, genfit::Track *itrack );
  void FillTrackGeometry ( StTrack             *otrack, genfit::Track *itrack, double zplane, int io );
  void FillTrackDcaGeometry ( StTrack             *otrack, genfit::Track *itrack );
  void FillTrackFitTraits( StTrack             *otrack, genfit::Track *itrack );
  void FillTrackMatches  ( StTrack             *otrack, genfit::Track *itrack );

  ClassDef(StgMaker, 0);
};

#endif
