#ifndef ST_FWD_TRACK_MAKER_H
#define ST_FWD_TRACK_MAKER_H

#include "StChain/StMaker.h"

#include "GenFit/Track.h"

#ifndef __CINT__
#include "StFwdTrackMaker/XmlConfig/XmlConfig.h"
#endif

namespace KiTrack {
class IHit;
};

class ForwardTracker;
class ForwardHitLoader;
class StarFieldAdaptor;

class StGlobalTrack;
class StRnDHitCollection;
class StTrack;
class StTrackDetectorInfo;
class SiRasterizer;
class McTrack;

// ROOT includes
#include "TNtuple.h"
#include "TTree.h"
// STL includes
#include <vector>
#include <memory>

const size_t MAX_TREE_ELEMENTS = 10000;

class StFwdTrackMaker : public StMaker {

    ClassDef(StFwdTrackMaker, 0);

  public:
    StFwdTrackMaker();
    ~StFwdTrackMaker(){/* nada */};

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

    // I could not get the library generation to succeed with these.
    // so I have removed them
    #ifndef __CINT__
        jdb::XmlConfig xfg;

        void loadMcTracks( std::map<int, std::shared_ptr<McTrack>> &mcTrackMap );
        void loadStgcHits( std::map<int, std::shared_ptr<McTrack>> &mcTrackMap, std::map<int, std::vector<KiTrack::IHit *>> &hitMap, int count = 0 );
        void loadStgcHitsFromGEANT( std::map<int, std::shared_ptr<McTrack>> &mcTrackMap, std::map<int, std::vector<KiTrack::IHit *>> &hitMap, int count = 0 );
        void loadStgcHitsFromStEvent( std::map<int, std::shared_ptr<McTrack>> &mcTrackMap, std::map<int, std::vector<KiTrack::IHit *>> &hitMap, int count = 0 );
        void loadFstHits( std::map<int, std::shared_ptr<McTrack>> &mcTrackMap, std::map<int, std::vector<KiTrack::IHit *>> &hitMap, int count = 0 );
        void loadFstHitsFromGEANT( std::map<int, std::shared_ptr<McTrack>> &mcTrackMap, std::map<int, std::vector<KiTrack::IHit *>> &hitMap, int count = 0 );
        void loadFstHitsFromStEvent( std::map<int, std::shared_ptr<McTrack>> &mcTrackMap, std::map<int, std::vector<KiTrack::IHit *>> &hitMap, int count = 0 );
    #endif
    

    // Fill StEvent
    void FillEvent();
    void FillDetectorInfo(StTrackDetectorInfo *info, genfit::Track *track, bool increment);
    void FillTrack(StTrack *otrack, genfit::Track *itrack, const Seed_t &iseed, StTrackDetectorInfo *info);
    void FillTrackFlags(StTrack *otrack, genfit::Track *itrack);
    void FillTrackGeometry(StTrack *otrack, genfit::Track *itrack, double zplane, int io);
    void FillTrackDcaGeometry ( StGlobalTrack    *otrack, genfit::Track *itrack );
    void FillTrackFitTraits(StTrack *otrack, genfit::Track *itrack);
    void FillTrackMatches(StTrack *otrack, genfit::Track *itrack);
};

#endif
