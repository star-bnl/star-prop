#ifndef __StgMaker_h__
#define __StgMaker_h__

#include "StMaker.h"

#include "GenFit/Track.h"

namespace KiTrack {
class IHit;
};

class ForwardTracker;
class ForwardHitLoader;
class StarFieldAdaptor;

class StRnDHitCollection;
class StTrack;
class StTrackDetectorInfo;
class SiRasterizer;

#include "TNtuple.h"
#include "TTree.h"
#include <vector>

const size_t MAX_TREE_ELEMENTS = 10000;

class StgMaker : public StMaker {

    ClassDef(StgMaker, 0);

  public:
    StgMaker();
    ~StgMaker(){/* nada */};

    int Init();
    int Finish();
    int Make();
    void Clear(const Option_t *opts = "");

    enum { kInnerGeometry,
           kOuterGeometry };

    void SetConfigFile(std::string n) {
        mConfigFile = n;
    }
    void GenerateTree(bool _genTree) { mGenTree = _genTree; }

  private:
  protected:
    ForwardTracker *mForwardTracker;
    ForwardHitLoader *mForwardHitLoader;
    StarFieldAdaptor *mFieldAdaptor;

    SiRasterizer *mSiRasterizer;

    typedef std::vector<KiTrack::IHit *> Seed_t;

    std::map<std::string, TH1 *> histograms;
    TFile *mlFile;
    TTree *mlTree;
    bool mGenTree;
    std::string mConfigFile;

    float mlt_x[MAX_TREE_ELEMENTS], mlt_y[MAX_TREE_ELEMENTS], mlt_z[MAX_TREE_ELEMENTS];
    int mlt_n, mlt_nt, mlt_tid[MAX_TREE_ELEMENTS], mlt_vid[MAX_TREE_ELEMENTS], mlt_hpt[MAX_TREE_ELEMENTS], mlt_hsv[MAX_TREE_ELEMENTS];
    float mlt_pt[MAX_TREE_ELEMENTS], mlt_eta[MAX_TREE_ELEMENTS], mlt_phi[MAX_TREE_ELEMENTS];
    std::map<string, std::vector<float>> mlt_crits;
    std::map<string, std::vector<int>> mlt_crit_track_ids;

    // Fill StEvent
    void FillEvent();
    void FillDetectorInfo(StTrackDetectorInfo *info, genfit::Track *track, bool increment);
    void FillTrack(StTrack *otrack, genfit::Track *itrack, const Seed_t &iseed, StTrackDetectorInfo *info);
    void FillTrackFlags(StTrack *otrack, genfit::Track *itrack);
    void FillTrackGeometry(StTrack *otrack, genfit::Track *itrack, double zplane, int io);
    void FillTrackDcaGeometry(StTrack *otrack, genfit::Track *itrack);
    void FillTrackFitTraits(StTrack *otrack, genfit::Track *itrack);
    void FillTrackMatches(StTrack *otrack, genfit::Track *itrack);
};

#endif
