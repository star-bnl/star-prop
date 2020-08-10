
#ifndef ST_FTT_SIMULATOR_MAKER_H
#define ST_FTT_SIMULATOR_MAKER_H

class g2t_emc_hit_st;
class StFtsHit;
class StEvent;

#include "StChain/StMaker.h"
#include <vector>

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TNtuple.h"

class StRnDHit;

class StFTTSimulatorMaker : public StMaker {
  public:
    explicit StFTTSimulatorMaker(const Char_t *name = "fttSim");
    virtual ~StFTTSimulatorMaker() {}
    Int_t Make();
    int Init();
    int Finish() {
        if (ttree && fTuple) {
            fTuple->cd();
            ttree->Write();
            fTuple->Close();
        }
        return kStOk;
    }
    virtual const char *GetCVS() const;

    void setDiskRotation(int disk, float degrees) {

        const float deg_to_radians = 3.1415926 / 180.0;
        if (9 == disk)
            sTGC_disk9_theta = degrees * deg_to_radians;
        if (10 == disk)
            sTGC_disk10_theta = degrees * deg_to_radians;
        if (11 == disk)
            sTGC_disk11_theta = degrees * deg_to_radians;
        if (12 == disk)
            sTGC_disk12_theta = degrees * deg_to_radians;
        return;
    }

  private:
    void fillThinGapChambers(StEvent *event);

    int iEvent;
    TFile *fTuple;
    TTree *ttree;
    // MC Level hit info on all detectors
    float tree_x[10000];
    float tree_y[10000];
    float tree_z[10000];
    float tree_pt[10000];
    float tree_eta[10000];
    float tree_phi[10000];
    int tree_vid[10000];
    int tree_tid[10000];
    int tree_n;

    // RECO Level without ghosts
    float tree_rx[10000];
    float tree_ry[10000];
    float tree_rz[10000];
    int tree_rvid[10000];
    int tree_rtid[10000];
    int tree_rn;

    // Ghost hits (includes real hits!)
    float tree_gx[10000];
    float tree_gy[10000];
    float tree_gz[10000];
    int tree_gvid[10000];
    int tree_gtid1[10000];
    int tree_gtid2[10000];
    int tree_gn;

    TH2F *hGlobalYX;
    TH2F *hOctantYX;

    TH2F *hOctantWireYX;
    TH2F *hOctantStripYX;

    TH2F *hWireDeltasX;
    TH2F *hWireDeltasY;
    TH2F *hStripDeltasX;
    TH2F *hStripDeltasY;

    TH2F *hWirePullsX;
    TH2F *hWirePullsY;
    TH2F *hStripPullsX;
    TH2F *hStripPullsY;

    TH2F *hPointsPullsX;
    TH2F *hPointsPullsY;

    bool mPointHits;
    //table to keep pointer to hit for each disc, r & phi strips
    // StRnDHit* _map[6][64][1536];
    //  double enrsum[6][64][1536];
    // double enrmax[6][64][1536];

    // convert x, y to quandrant and local X, Y
    // quadrants are
    // 0 1
    // 3 2
    void sTGCGlobalToLocal(float x, float y, int disk, int &quad, float &localX, float &localY);
    void sTGCLocalToGlobal(float localX, float localY, int disk, int quad, float &globalX, float &globalY);
    // long long sTGCMapLocal( float x, float y, int face, float &wireX, float &wireY );
    bool sTGCOverlaps(StRnDHit *hitA, StRnDHit *hitB);
    void sTGCQuadBottomLeft(int disk, int quad, float &bottom, float &left);
    float diskOffset(int disk);
    float diskRotation(int disk);

    void rot(float theta, float x, float y, float &xp, float &yp) {
        xp = x * cos(theta) - y * sin(theta);
        yp = x * sin(theta) + y * cos(theta);
    }

    const double STGC_BEAM_CUT_OUT = 6.0;   // cm
    const double STGC_QUAD_WIDTH = 60.0;    // cm
    const double STGC_QUAD_HEIGHT = 60.0;   // cm
    const double STGC_WIRE_WIDTH = 0.32;    // cm
    const double STGC_SIGMA_X = 0.01;       // 100 microns
    const double STGC_SIGMA_Y = 0.01;       // 100 microns
    const double STGC_SIGMA_Z = 0.001;      // 10 microns
    const double STGC_WIRE_LENGTH = 15.0;   // cm
    const bool STGC_MAKE_GHOST_HITS = true; //should be moved to run-time opt

    float sTGC_disk9_theta = 0.0f;
    float sTGC_disk10_theta = 0.0f;
    float sTGC_disk11_theta = 0.0f;
    float sTGC_disk12_theta = 0.0f;

    int sTGCNRealPoints = 0;
    int sTGCNGhostPoints = 0;

    ClassDef(StFTTSimulatorMaker, 0)
};

inline const char *StFTTSimulatorMaker::GetCVS() const {
    static const char cvs[] = "Tag $Name:  $ $Id: StFTTSimulatorMaker.h,v 1.1.2.3 2018/06/07 16:18:49 jwebb Exp $ built " __DATE__ " " __TIME__;
    return cvs;
}

#endif
