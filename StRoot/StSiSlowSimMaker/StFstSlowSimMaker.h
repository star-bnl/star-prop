#ifndef ST_FST_SLOW_SIM_MAKER_H
#define ST_FST_SLOW_SIM_MAKER_H

class g2t_emc_hit_st;
class StFtsHit;
class StEvent;
class StRnDHitCollection;

#include "StChain/StMaker.h"
#include <vector>

#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"

class StRnDHit;

class StFstSlowSimMaker : public StMaker {
  public:
    explicit StFstSlowSimMaker(const Char_t *name = "fstSlowSim");
    virtual ~StFstSlowSimMaker() {}
    Int_t Make();
    int Init();
    int Finish();
    virtual const char *GetCVS() const;
    /// Define segmentation
    void setPixels(int numR = 8, int numSec = 12, int numPhi = 128) {
        mNumR = numR;
        mNumSEC = numSec;
        mNumPHI = numPhi;
    }
    /// Set offset for each disk ( x=R*cos(idisk*60 degrees), y=R*sin(...) )
    void setRaster(float R = 1.0) { mRaster = R; }
    /// Set min/max active radii for each disk
    void setDisk(const int i, const float rmn, const float rmx);
    /// Set even spacing in eta (defaults to even in r)
    void setConstEta() { mConstEta = true; }
    void setInEfficiency(float ineff = 0.1) { mInEff = ineff; }
    void setQAFileName(TString filename = 0.1) { mQAFileName = filename; }

    /// Enable / disable specified disk (disks counted from 1)
    void setActive(const int disk, const bool flag = true) { mEnable[disk - 1] = flag; }
    /// Point making in each pad (strip,wire)-->(x,y)
    void setPointHits() { mPointHits = true; }

    void setStripWidth(const float width = 0.3) { mStripWidth = width; }
    void setWireSpacing(const float space = 0.15) { mWireSpacing = space; }
    void setWindow(const float window = 3.0) { mWindow = window; }

    void setStereo(const float stereo = 2 * TMath::Pi() / 8 / 6) { mStereo = stereo; }

    void setStagger(const float stagger = 3.0) { mStagger = 3.0; }

    void setAmbiguity(const bool a = true) { mAmbiguity = a; }

    void setCrossTalkSwitch() { mCrossTalkSwitch = true; }
    
    void setREffSwitch() { mREffSwitch = true; }

    void setNoiseSwitch() { mNoiseSwitch = true; }

    void setRstripEff(float r0 = 0.0, float r1 = 0.0, float r2 = 0.0, float r3 = 0.0, float r4 = 0.0, float r5 = 0.0, float r6 = 0.0, float r7 = 0.0) {
        mRstripEff[0] = r0;
	mRstripEff[1] = r1;	
	mRstripEff[2] = r2;
	mRstripEff[3] = r3;
	mRstripEff[4] = r4;
	mRstripEff[5] = r5;
	mRstripEff[6] = r6;
	mRstripEff[7] = r7;
    }

  private:
    Int_t getDetectorId(const g2t_emc_hit_st &hit) const;

    void fillSilicon(StEvent *event);
    void fillThinGapChambers(StEvent *event);
   
    StRnDHitCollection *hitCollection = nullptr;

    bool mEnable[12];

    int mNumR;
    int mNumPHI;
    int mNumSEC;
     
    bool mCrossTalkSwitch;
    int getCrossTalkRSegment(int irstrip);    

    float mRaster;

    float mInEff;
    
    bool mREffSwitch;
    float mRstripEff[8];

    bool mNoiseSwitch;

    TString mQAFileName;

    TH3F *hTrutHitYXDisk;
    TH2F *hTrutHitRDisk;
    TH2F *hTrutHitRShower[3];
    TH2F *hTrutHitPhiDisk;
    TH2F *hTrutHitPhiZ;
    TH3F *hRecoHitYXDisk;
    TH2F *hRecoHitRDisk;
    TH2F *hRecoHitPhiDisk;
    TH2F *hRecoHitPhiZ;
    TH2F *hGlobalDRDisk;
    TH1F *hGlobalZ;

    TH2F *hRRcVsRMc[8];
    TH1F *hRMc[8];
    TH1F *hRRc[8];

    TH1F *hGlobalDeltaR;
    TH1F *hGlobalDeltaRAll[8];
    TH1F *hGlobalDeltaPhi;
    TH1F *hGlobalDeltaPhiAll[8];

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
    bool mAmbiguity;
    
    TH2F *h2LocalXY;
    TH2F *h2LocalSmearedXY;
    TH2F *h2LocalDeltaXY;
    TH3F *h3LocalDeltaXYDisk;
    TH3F *h3LocalDeltaXYR;
    TH2F *h2GlobalXY;
    TH2F *h2GlobalSmearedXY;
    TH3F *h3GlobalXYDisk;
    TH3F *h3GlobalXYR;
    TH3F *h3GlobalSmearedXYDisk;
    TH3F *h3GlobalSmearedXYR;
    TH2F *h2GlobalDeltaXY;
    TH3F *h3GlobalDeltaXYDisk;
    TH3F *h3GlobalDeltaXYR;
    TFile *fOut;

    float mStripWidth;
    float mWireSpacing;
    float mWindow; // for filtering combinatorics
    float mStereo;
    float mStagger;

    bool mConstEta;

    ClassDef(StFstSlowSimMaker, 0)
};

inline const char *StFstSlowSimMaker::GetCVS() const {
    static const char cvs[] = "Tag $Name:  $ $Id: StFstSlowSimMaker.h,v 1.1 2018/11/06 18:56:05 jdb Exp $ built " __DATE__ " " __TIME__;
    return cvs;
}

#endif
