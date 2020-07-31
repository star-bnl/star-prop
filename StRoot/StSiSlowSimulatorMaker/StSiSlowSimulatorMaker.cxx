#include "StSiSlowSimulatorMaker.h"

#include "St_base/StMessMgr.h"

#include "StEvent/StEvent.h"
#include "StEvent/StRnDHit.h"
#include "StEvent/StRnDHitCollection.h"

#include "tables/St_g2t_fts_hit_Table.h"
#include "tables/St_g2t_track_Table.h"

#include "StThreeVectorF.hh"

#include "TCanvas.h"
#include "TCernLib.h"
#include "TH2F.h"
#include "TLine.h"
#include "TRandom3.h"
#include "TString.h"
#include "TVector2.h"
#include "TVector3.h"

#include <array>

TCanvas *canvas = 0;
StMatrixF Hack1to6(const StHit *stHit);

constexpr float PI = atan2(0.0, -1.0);
constexpr float SQRT12 = sqrt(12.0);

const float OCTANT_WIDTH_PHI = PI / 4;
const float OCTANT_GLOBAL_PHI_MIN[] = {-PI / 8, PI / 8, 3 * PI / 8, 5 * PI / 8, 7 * PI / 8, 9 * PI / 8, 11 * PI / 8, 13 * PI / 8};
const float OCTANT_GLOBAL_PHI_MAX[] = {PI / 8, 3 * PI / 8, 5 * PI / 8, 7 * PI / 8, 9 * PI / 8, 11 * PI / 8, 13 * PI / 8, 15 * PI / 8};

const float OCTANT_XMIN[] = {6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f}; // octant size in each disk...
const float OCTANT_XMAX[] = {42.0f, 42.0f, 66.0f, 66.0f, 66.0f, 66.0f};

const float PAD_WIDTH = 6.0f;
const float STRIP_WIDTH = 0.3f;  // must divide nicly into 6cm
const float WIRE_SPACING = 0.3f; // ditto

//
// Disk segmentation
//
// float RMIN[] = {   0.85 * 2.56505,   0.85 * 3.41994,   0.85 * 4.27484,  0.85 * 5.13010, 0.85 * 5.985, 0.85 * 6.83988 };
// float RMAX[] = {  1.15 * 11.56986,  1.15 * 15.42592,  1.15 * 19.28199, 1.15 * 23.13971, 1.15 * 26.99577, 1.15 * 30.84183 };
// float RMIN[] = {   0.85 * 2.0,   0.85 * 2.07,   0.85 * 2.59,  0.85 * 3.11, 0.85 * 3.63, 0.85 * 4.15};
// // float RMAX[] = {  1.15 * 19.3,  1.15 * 25.7,  1.15 * 32.1, 1.15 * 32.25, 1.15 * 32.25, 1.15 * 32.25};
// float RMAX[] = {  1.15 * 15.0,  1.15 * 20.0,  1.15 * 25.0, 1.15 * 32.25, 1.15 * 32.25, 1.15 * 32.25};
//
// float RMIN[] = {   0.85 * 4.3,   0.85 * 4.3,   0.85 * 4.3,  0.85 * 5.0, 0.85 * 5.0, 0.85 * 5.0};
// float RMAX[] = {  1.15 * 15.0,  1.15 * 25.0,  1.15 * 25.0, 1.15 * 30.5, 1.15 * 30.5, 1.15 * 30.5};
//
float RMIN[] = {0.95 * 4.3, 0.95 * 4.3, 0.95 * 4.3, 0.95 * 5.0, 0.95 * 5.0, 0.95 * 5.0};
float RMAX[] = {1.05 * 15.0, 1.05 * 25.0, 1.05 * 25.0, 1.05 * 28.0, 1.05 * 28.0, 1.05 * 28.0};

//
// next is for test with the default seg, from 5.0 to 30.5. divide even
// for (int i=0;i<9;i++) cout << 5*0.85+(30.5*1.15-5*0.85)/8.*i << " , ";
// float RSegment[] = {4.25 , 8.10312 , 11.9562 , 15.8094 , 19.6625 , 23.5156 , 27.3687 , 31.2219 , 35.075};
//
//NEXT IS only for disk ARRAY 456 with the radius from 5 to 30.5
// for (int i=0;i<8;i++)  cout << 6.5+(30.5-6.5)/7.*i << " , ";
// for 5.0 - 6.5, then the other is divided uniform
// float RSegment[] = {5.0*0.85, 6.5 , 9.92857 , 13.3571 , 16.7857 , 20.2143 , 23.6429 , 27.0714 , 30.5 * 1.15};
//
//NEXT IS only for disk ARRAY 456 with the radius from 5 to 28. half and half 11.5
//for first half, first one is 1.5 cm, later three divide even
//second half, divide even
// float RSegment[] = {5.0*0.95, 6.5 , 9.833, 13.16, 16.5 , 19.375, 22.25, 25.125 , 28.0 * 1.05};

//NEXT IS only for disk ARRAY 456 with the radius from 5 to 28.
float RSegment[] = {5., 7.875, 10.75, 13.625, 16.5, 19.375, 22.25, 25.125, 28.};

void StSiSlowSimulatorMaker::setDisk(const int i, const float rmn, const float rmx) {
    RMIN[i] = rmn;
    RMAX[i] = rmx;
}

// helper functions

int globalToOctant(const float &xglobal, /* input */
                   const float &yglobal, /* input */
                   const float &stereo,  /* input */
                   float &xoctant,       /* output */
                   float &yoctant /* output */) {

   //
   // takes as input a stereo angle, which is subtracted the phibin boundaries
   //

   // Get global phi and map it between the min and max octant boundaries
   float phi_global = TMath::ATan2(yglobal, xglobal);

   while (phi_global < OCTANT_GLOBAL_PHI_MIN[0] - stereo)
      phi_global += 2 * PI;
   while (phi_global >= OCTANT_GLOBAL_PHI_MAX[7] - stereo)
      phi_global -= 2 * PI;

   // Find which octant we are in
   int octant = 0;
   for (octant = 0; octant < 7; octant++) {
      float phi_min = OCTANT_GLOBAL_PHI_MIN[octant] - stereo;
      float phi_max = OCTANT_GLOBAL_PHI_MAX[octant] - stereo;
      //      std::cout << octant << " mn=" << phi_min << " mx=" << phi_max << " phig=" << phi_global << std::endl;
      if (phi_global >= phi_min && phi_global < phi_max)
         break;
   }

   float phi_local = phi_global - octant * OCTANT_WIDTH_PHI + stereo;
   //  float rot_angle = phi_local  - phi_global;
   float rot_angle = -octant * OCTANT_WIDTH_PHI + stereo;

   xoctant = xglobal * TMath::Cos(rot_angle) - yglobal * TMath::Sin(rot_angle);
   yoctant = xglobal * TMath::Sin(rot_angle) + yglobal * TMath::Cos(rot_angle);

   return octant;
}

int octantToGlobal(const float &xoctant, /* input */
                   const float &yoctant, /* input */
                   const int &octant,    /* input */
                   const float &stereo,  /* input */
                   float &xglobal,       /* output */
                   float &yglobal /* output */,
                   float *Ematrix = 0 /* optional error matrix in triangular form */
) {

   // Get local phi
   float phi_local = TMath::ATan2(yglobal, xglobal);

   // rotation angle back to global
   float rot_angle = octant * OCTANT_WIDTH_PHI - stereo;

   xglobal = xoctant * TMath::Cos(rot_angle) - yoctant * TMath::Sin(rot_angle);
   yglobal = xoctant * TMath::Sin(rot_angle) + yoctant * TMath::Cos(rot_angle);

   if (Ematrix) {
      float Eout[3];
      float R[] = {float(TMath::Cos(rot_angle)), float(-TMath::Sin(rot_angle)),
                  float(TMath::Sin(rot_angle)), float(TMath::Cos(rot_angle))};

      TCL::trasat(R, Ematrix, Eout, 2, 2);

      Ematrix[0] = Eout[0];
      Ematrix[1] = Eout[1];
      Ematrix[2] = Eout[2];
   }

   return octant;
}

const bool verbose = true;
const bool merge_hits = true;

StSiSlowSimulatorMaker::StSiSlowSimulatorMaker(const Char_t *name)
   : StMaker(name),
   mNumR(64),
   mNumPHI(128),
   mNumSEC(12),
   mRaster(0),
   mInEff(0),
   mQAFileName(0),
   mEnable({true, true, true, true, true, true, true, true, true, true, true, true}),
   hTrutHitYXDisk(0),
   hTrutHitRDisk(0),
   hTrutHitRShower({0}),
   hTrutHitPhiDisk(0),
   hTrutHitPhiZ(0),
   hRecoHitYXDisk(0),
   hRecoHitRDisk(0),
   hRecoHitPhiDisk(0),
   hRecoHitPhiZ(0),
   hGlobalDRDisk(0),
   hGlobalZ(0),

   hRRcVsRMc({0, 0, 0, 0, 0, 0, 0, 0}),
   hRMc({0, 0, 0, 0, 0, 0, 0, 0}),
   hRRc({0, 0, 0, 0, 0, 0, 0, 0}),

   hGlobalDeltaR(0),   
   hGlobalDeltaPhi(0), 
   hGlobalDeltaRAll({0, 0, 0, 0, 0, 0, 0, 0}),   
   hGlobalDeltaPhiAll({0, 0, 0, 0, 0, 0, 0, 0}),   

   hOctantYX(0),
   hOctantWireYX(0),
   hOctantStripYX(0),
   hWireDeltasX(0),
   hWireDeltasY(0),
   hStripDeltasX(0),
   hStripDeltasY(0),
   hWirePullsX(0),
   hWirePullsY(0),
   hStripPullsX(0),
   hStripPullsY(0),
   hPointsPullsX(0),
   hPointsPullsY(0),
   fOut(0),
   h2LocalXY(0),
   h2LocalSmearedXY(0),
   h2LocalDeltaXY(0),
   h3LocalDeltaXYDisk(0),
   h3LocalDeltaXYR(0),
   h2GlobalXY(0),
   h2GlobalSmearedXY(0),
   h3GlobalXYDisk(0),
   h3GlobalXYR(0),
   h3GlobalSmearedXYDisk(0),
   h3GlobalSmearedXYR(0),
   h2GlobalDeltaXY(0),
   h3GlobalDeltaXYDisk(0),
   h3GlobalDeltaXYR(0),
   mPointHits(false),
   mAmbiguity(true),
   mStripWidth(0.3),
   mWireSpacing(0.15),
   mWindow(3.0),
   mStereo(2 * TMath::Pi() / 8 / 6),
   mStagger(0.0),
   mCrossTalkSwitch(true),
   mREffSwitch(true),
   mRstripEff({0.95, 0.95, 0.95, 0.95, 0.90, 0.90, 0.90, 0.90}),
   mNoiseSwitch(false),
   mConstEta(false) {}

int StSiSlowSimulatorMaker::Init() {

   AddHist(hTrutHitYXDisk = new TH3F("hTrutHitYXDisk", "Global hits before segmentation", 151, -75.5, 75.5, 151, -75.5, 75.5, 10, 0, 10));
   AddHist(hTrutHitRDisk = new TH2F("hTrutHitRDisk", "Global hits before segmentation", 400, 0, 40, 10, 0, 10));
   AddHist(hTrutHitRShower[0] = new TH2F("hTrutHitRShower_4", "Global hits before segmentation", 400, 0, 40, 20, -10, 10));
   AddHist(hTrutHitRShower[1] = new TH2F("hTrutHitRShower_5", "Global hits before segmentation", 400, 0, 40, 20, -10, 10));
   AddHist(hTrutHitRShower[2] = new TH2F("hTrutHitRShower_6", "Global hits before segmentation", 400, 0, 40, 20, -10, 10));
   AddHist(hTrutHitPhiDisk = new TH2F("hTrutHitPhiDisk", "Global hits before segmentation", 360, 0, 360, 10, 0, 10));
   AddHist(hTrutHitPhiZ = new TH2F("hTrutHitPhiZ", "Global hits before segmentation", 360, 0, 360, 6000, 0, 600));
   AddHist(hRecoHitYXDisk = new TH3F("hRecoHitYXDisk", "Global hits after segmentation", 151, -75.5, 75.5, 151, -75.5, 75.5, 10, 0, 10));
   AddHist(hRecoHitRDisk = new TH2F("hRecoHitRDisk", "Global hits after segmentation", 400, 0, 40, 10, 0, 10));
   AddHist(hRecoHitPhiDisk = new TH2F("hRecoHitPhiDisk", "Global hits after segmentation", 360, 0, 360, 10, 0, 10));
   AddHist(hRecoHitPhiZ = new TH2F("hRecoHitPhiZ", "Global hits after segmentation", 360, 0, 360, 6000, 0, 600));
   AddHist(hGlobalDRDisk = new TH2F("hGlobalDRDisk", "; Reco. r - MC r [cm]; Events;", 1000, -50, 50, 10, 0, 10));
   AddHist(hGlobalZ = new TH1F("hGlobalZ", "; Z [cm]; Events;", 6000, 0, 600));

   AddHist(hGlobalDeltaR = new TH1F("hGlobalDeltaR", "R Residual; counts", 100, -11.75, 11.75)); // cm
   AddHist(hGlobalDeltaPhi = new TH1F("hGlobalDeltaPhi", "Phi Residual; counts", 100, -0.01, 0.01)); // rad   
   for(int i_rstrip = 0; i_rstrip < 8; ++i_rstrip){
      std::string HistName;
      std::string HistTitle;
      HistName = Form("hGlobalDeltaRRstrip%d", i_rstrip);
      HistTitle = Form("R Residual Rstrip %d; counts", i_rstrip);
      AddHist(hGlobalDeltaRAll[i_rstrip] = new TH1F(HistName.c_str(), HistTitle.c_str(), 100, -11.75, 11.75));
      HistName = Form("hGlobalDeltaPhiRstrip%d", i_rstrip);
      HistTitle = Form("Phi Residual Rstrip %d; counts", i_rstrip);
      AddHist(hGlobalDeltaPhiAll[i_rstrip] = new TH1F(HistName.c_str(), HistTitle.c_str(), 100, -0.01, 0.01));

      HistName = Form("hRRcVsRMcRstrip%d", i_rstrip);
      HistTitle = Form("R_{RC} vs R_{MC} Rstrip %d; R_{MC}; R_{RC}", i_rstrip);
      AddHist(hRRcVsRMc[i_rstrip] = new TH2F(HistName.c_str(), HistTitle.c_str(), 100, 4.75, 28.25, 100, 4.75, 28.25));

      HistName = Form("hRMcRstrip%d", i_rstrip);
      HistTitle = Form("R_{MC} Rstrip %d; R_{MC}; counts", i_rstrip);
      AddHist(hRMc[i_rstrip] = new TH1F(HistName.c_str(), HistTitle.c_str(), 100, 4.75, 28.25)); // cm

      HistName = Form("hRRcRstrip%d", i_rstrip);
      HistTitle = Form("R_{RC} Rstrip %d; R_{RC}; counts", i_rstrip);
      AddHist(hRRc[i_rstrip] = new TH1F(HistName.c_str(), HistTitle.c_str(), 100, 4.75, 28.25)); // cm

   }
   AddHist(hOctantYX = new TH2F("hOctantYX", "Octant hits before segmentation", 151, -75.5, 75.5, 151, -75.5, 75.5));

   AddHist(hOctantStripYX = new TH2F("hOctantStripYX", "Octant hits with strip segmentation", 141, -0.25, 70.25, 141, -35.25, 35.25));
   AddHist(hOctantWireYX = new TH2F("hOctantWireYX", "Octant hits with wire segmentation", 141, -0.25, 70.25, 141, -35.25, 35.25));

   // Book deltas for FTS thin gap chamber wheels
   AddHist(hWireDeltasX = new TH2F("hWireDeltasX", ";deltaX;sTGC wheel", 81, -4.05, 4.05, 7, -0.5, 6.5));
   AddHist(hWireDeltasY = new TH2F("hWireDeltasY", ";deltaY;sTGC wheel", 81, -4.05, 4.05, 7, -0.5, 6.5));

   AddHist(hStripDeltasX = new TH2F("hStripDeltasX", ";deltaX;sTGC wheel", 81, -4.05, 4.05, 7, -0.5, 6.5));
   AddHist(hStripDeltasY = new TH2F("hStripDeltasY", ";deltaY;sTGC wheel", 81, -40.5, 40.5, 7, -0.5, 6.5));

   // Book pulls for FTS thin gap chamber wheels
   AddHist(hWirePullsX = new TH2F("hWirePullsX", ";pullX;sTGC wheel", 81, -4.05, 4.05, 7, -0.5, 6.5));
   AddHist(hWirePullsY = new TH2F("hWirePullsY", ";pullY;sTGC wheel", 81, -4.05, 4.05, 7, -0.5, 6.5));

   AddHist(hStripPullsX = new TH2F("hStripPullsX", ";pullX;sTGC wheel", 81, -4.05, 4.05, 7, -0.5, 6.5));
   AddHist(hStripPullsY = new TH2F("hStripPullsY", ";pullY;sTGC wheel", 81, -4.05, 4.05, 7, -0.5, 6.5));

   AddHist(hPointsPullsX = new TH2F("hPointsPullsX", ";pullX;sTGC wheel", 81, -4.05, 4.05, 7, -0.5, 6.5));
   AddHist(hPointsPullsY = new TH2F("hPointsPullsY", ";pullY;sTGC wheel", 81, -4.05, 4.05, 7, -0.5, 6.5));

   //fOut = new TFile("FastSimu.QA.root", "RECREATE");
   fOut = new TFile(mQAFileName.Data(), "RECREATE");
   AddHist(h2LocalXY = new TH2F("h2LocalXY", ";localX; localY", 800, -4.0, 4.0, 800, -4.0, 4.0));
   AddHist(h2LocalSmearedXY = new TH2F("h2LocalSmearedXY", ";localSmearedX; localSmearedY", 800, -4.0, 4.0, 800, -4.0, 4.0));
   AddHist(h2LocalDeltaXY = new TH2F("h2LocalDeltaXY", ";localDeltaX; localDeltaY", 1000, -0.05, 0.05, 1000, -0.1, 0.1));
   AddHist(h3LocalDeltaXYDisk = new TH3F("h3LocalDeltaXYDisk", ";localDeltaX; localDeltaY; Disk", 300, -3, 3, 300, -1, 1, 10, 0, 10));
   AddHist(h3LocalDeltaXYR = new TH3F("h3LocalDeltaXYR", ";localDeltaX; localDeltaY; R", 300, -0.3, 0.3, 300, -3, 3, 100, 0, 30));
   AddHist(h2GlobalXY = new TH2F("h2GlobalXY", ";globalX; globalY", 1510, -75.5, 75.5, 1510, -75.5, 75.5));
   AddHist(h2GlobalSmearedXY = new TH2F("h2GlobalSmearedXY", ";globalSmearedX; globalSmearedY", 1510, -75.5, 75.5, 1510, -75.5, 75.5));
   AddHist(h3GlobalXYDisk = new TH3F("h3GlobalXYDisk", ";globalX; globalY; disk", 1510, -75.5, 75.5, 1510, -75.5, 75.5, 10, 0, 10));
   // AddHist(h3GlobalXYR= new TH3F("h3GlobalXYR", ";globalX; globalY; R",   1510, -75.5, 75.5, 1510, -75.5, 75.5, 100, 0, 30));
   AddHist(h3GlobalSmearedXYDisk = new TH3F("h3GlobalSmearedXYDisk", ";globalSmearedX; globalSmearedY; disk", 1510, -75.5, 75.5, 1510, -75.5, 75.5, 10, 0, 10));
   // AddHist(h3GlobalSmearedXYR= new TH3F("h3GlobalSmearedXYR", ";globalSmearedX; globalSmearedY; R",   1510, -75.5, 75.5, 1510, -75.5, 75.5, 100, 0, 30));
   AddHist(h2GlobalDeltaXY = new TH2F("h2GlobalDeltaXY", ";globalDeltaX; globalDeltaY", 151, -75.5, 75.5, 151, -75.5, 75.5));
   AddHist(h3GlobalDeltaXYDisk = new TH3F("h3GlobalDeltaXYDisk", ";globalDeltaX; globalDeltaY; Disk", 151, -75.5, 75.5, 151, -75.5, 75.5, 10, 0, 10));
   AddHist(h3GlobalDeltaXYR = new TH3F("h3GlobalDeltaXYR", ";globalDeltaX; globalDeltaY; R", 151, -75.5, 75.5, 151, -75.5, 75.5, 100, 0, 30));

   return StMaker::Init();
}

Int_t StSiSlowSimulatorMaker::Make() {
   LOG_DEBUG << "StSiSlowSimulatorMaker::Make" << endm;
   // Get the existing StEvent, or add one if it doesn't exist.
   StEvent *event = static_cast<StEvent *>(GetDataSet("StEvent"));
   if (!event) {
      event = new StEvent;
      AddData(event);
      LOG_DEBUG << "Creating StEvent" << endm;
   }
   // // Add an FTS collection to the event if one does not already exist.
   // if (!event->fsiCollection()) {
   //    event->setFtsCollection(new StFtsHitCollection);
   //    LOG_DEBUG << "Creating StFtsHitCollection" << endm;
   // }  // if

   if (0 == event->rndHitCollection()) {
      event->setRnDHitCollection(new StRnDHitCollection());
      LOG_DEBUG << "Creating StRnDHitCollection for FTS" << endm;
   }

   // Digitize GEANT FTS hits
   fillSilicon(event);
   //    event->rndHitCollection()->Print();
   // fillThinGapChambers(event);

   return kStOk;
}

/* Fill an event with StFtsHits. */
/* This should fill StFtsStrip for realistic simulator and let clustering fill StFtsHit */
/* For now skipping StFtsStrip and clustering, and fill StFtsHits directly here*/

void StSiSlowSimulatorMaker::fillSilicon(StEvent *event) {

   //    StFtsHitCollection * fsicollection = event->fsiCollection();
   StRnDHitCollection *fsicollection = event->rndHitCollection();

   /*static const*/ const int NDISC = 6;
   //static const int MAXR  =128; // JCW: let's give Stv best shot at this possible...
   //static const int MAXPHI=128*12;
   /*static const */ const int MAXR = mNumR;
   /*static const */ const int MAXPHI = mNumPHI * mNumSEC;

   //I guess this should be RSEG[NDISC][MAXR+1] array to give better R segements
   //For now this is just unform R segments regardless of disc
   //static const float RMIN[NDISC]={ 2.5, 2.5, 2.5, 2.5}; //hack this need to get right numbers
   //static const float RMAX[NDISC]={23.2,23.2
   //1-D histogram with a float per channel (see TH1 d,23.2,23.2}; //hack this need to get right numbers
   //    static const float PI=atan2(0.0,-1.0);
   //    static const float SQRT12=sqrt(12.0);
   /*
Rmin =     2.56505
Rmax =    11.56986
Rmin =     3.41994
Rmax =    15.42592
Rmin =     4.27484
Rmax =    19.28199
Rmin =     5.13010
Rmax =    23.13971
*/
   // static const float RMIN[] = {   2.56505,   3.41994,   4.27484,  5.13010, 5.985, 6.83988 };
   // static const float RMAX[] = {  11.56986,  15.42592,  19.28199, 23.13971, 26.99577, 30.84183 };
   // Constant eta segmentation
   static const float etaMn = 2.5;
   static const float etaMx = 4.0;
   // static const float etaMn = 2.0;
   // static const float etaMx = 4.5;
   static const float thetaMn = 2.0 * TMath::ATan(TMath::Exp(-etaMx));
   static const float thetaMx = 2.0 * TMath::ATan(TMath::Exp(-etaMn));
   static const int nbins = MAXR;
   static const float deta = (etaMx - etaMn) / nbins;

   // Raster each disk by 1mm, 60 degree offset for every disk
   static float X0[] = {0, 0, 0, 0, 0, 0};
   static float Y0[] = {0, 0, 0, 0, 0, 0};
   if (mRaster > 0)
      for (int i = 0; i < 6; i++) {
         X0[i] = mRaster * TMath::Cos(i * 60 * TMath::DegToRad());
         Y0[i] = mRaster * TMath::Sin(i * 60 * TMath::DegToRad());
      }

   //table to keep pointer to hit for each disc, r & phi strips
   StRnDHit *_map[NDISC][MAXR][MAXPHI];
   double ***enrsum = (double ***)malloc(NDISC * sizeof(double **));
   double ***enrmax = (double ***)malloc(NDISC * sizeof(double **));
   //memset( _map, 0, NDISC*MAXR*MAXPHI*sizeof(StRnDHit*) );
   //StRnDHit* ***_map = (StRnDHit* ***)malloc(NDISC*sizeof(StRnDHit* **));[NDISC][MAXR][MAXPHI];
   for (int id = 0; id < NDISC; id++) {
      enrsum[id] = (double **)malloc(MAXR * sizeof(double *));
      enrmax[id] = (double **)malloc(MAXR * sizeof(double *));
      for (int ir = 0; ir < MAXR; ir++) {
         enrsum[id][ir] = (double *)malloc(MAXPHI * sizeof(double));
         enrmax[id][ir] = (double *)malloc(MAXPHI * sizeof(double));
      }
   }

   for (int id = 0; id < NDISC; id++) {
      for (int ir = 0; ir < MAXR; ir++) {
         for (int ip = 0; ip < MAXPHI; ip++) {
               _map[id][ir][ip] = 0;
               enrsum[id][ir][ip] = 0;
               enrmax[id][ir][ip] = 0;
         }
      }
   }

   // Read the g2t table
   St_g2t_fts_hit *hitTable = static_cast<St_g2t_fts_hit *>(GetDataSet("g2t_fsi_hit"));
   if (!hitTable) {
     // LOG_INFO << "g2t_fsi_hit table is empty" << endm;
      return; // Nothing to do
   }           // if

   const Int_t nHits = hitTable->GetNRows();
   //LOG_DEBUG << "g2t_fsi_hit table has " << nHits << " hits" << endm;
   const g2t_fts_hit_st *hit = hitTable->GetTable();
   //    StPtrVecFtsHit hits; //temp storage for hits
   StPtrVecRnDHit hits;

   // track table
   St_g2t_track *trkTable = static_cast<St_g2t_track *>(GetDataSet("g2t_track"));
   if (!trkTable) {
     // LOG_INFO << "g2t_track table is empty" << endm;
      return; // Nothing to do
   }           // if

   const Int_t nTrks = trkTable->GetNRows();
   //LOG_DEBUG << "g2t_track table has " << nTrks << " tracks" << endm;
   const g2t_track_st *trk = trkTable->GetTable();

   gRandom->SetSeed(0);
  
   int count = 0;
   for (Int_t i = 0; i < nHits; ++i) {
      hit = (g2t_fts_hit_st *)hitTable->At(i);
      if (hit) {
         int volume_id = hit->volume_id;
//LOG_INFO << "volume_id = " << volume_id << endm;
         int d = volume_id / 1000;        // disk id
         int w = (volume_id % 1000) / 10; // wedge id
         int s = volume_id % 10;          // sensor id
       //  LOG_INFO << "d = " << d << ", w = " << w << ", s = " << s << endm;

         //     LOG_INFO << " volume id = " << d << endm;
         //if (d > 6) continue;   // skip large disks
         //if (false == mEnable[d - 1]) continue; // disk switched off

         float e = hit->de;
         int t = hit->track_p;

         trk = (g2t_track_st *)trkTable->At(t);
         int isShower = false;
         if (trk)
               isShower = trk->is_shower;

         float xc = X0[d - 1];
         float yc = Y0[d - 1];

         float x = hit->x[0];
         float y = hit->x[1];
         float z = hit->x[2];

         if (z > 200)
               continue; // skip large disks

         // rastered
         float xx = x - xc;
         float yy = y - yc;

         float r = sqrt(x * x + y * y);
         float p = atan2(y, x);

         // rastered
         float rr = sqrt(xx * xx + yy * yy);
         float pp = atan2(yy, xx);

         while (p < 0.0)
               p += 2.0 * PI;
         while (p >= 2.0 * PI)
               p -= 2.0 * PI;
         while (pp < 0.0)
               pp += 2.0 * PI;
         while (pp >= 2.0 * PI)
               pp -= 2.0 * PI;

         //LOG_INFO << "rr = " << rr << " pp=" << pp << endm;
         //LOG_INFO << "RMIN = " << RMIN[d - 1] << " RMAX= " << RMAX[d - 1] << endm;
         // compute point eta
         float theta = TMath::ATan2(rr, z); // rastered eta
         float eta = -TMath::Log(TMath::Tan(theta / 2));

         // Cuts made on rastered value
         if (rr < RMIN[d - 1] || rr > RMAX[d - 1])
               continue;
         //LOG_INFO << "rr = " << rr << endm;

         // Strip numbers on rastered value
         int ir = int(MAXR * (rr - RMIN[d - 1]) / (RMAX[d - 1] - RMIN[d - 1]));
         // LOG_INFO << "ir1 = " << ir << endm;
         for (int ii = 0; ii < MAXR; ii++){
               if (rr > RSegment[ii] && rr <= RSegment[ii + 1]){
                  ir = ii;
               }
         }
	
	 int ir_mc = ir;	 

	 if (mREffSwitch == true) {
	       double ran = gRandom->Uniform(0.0,1.0);
	       if (mRstripEff[ir_mc] < ran) continue; 
	 }      

	 if (mCrossTalkSwitch == true) { 
               if (ir_mc >= 0 && ir_mc <= 7) ir = getCrossTalkRSegment(ir_mc);
         }
	 
	 std::cout << "Rstrip MC = " << ir_mc << " Rstrip RC = " << ir << std::endl;
        
         /*
         if (mREffSwitch) {
  	       double ran = gRandom->Uniform(0.0,1.0);
	       if (mRstripEff[ir] < ran) continue; 
         }
         if (mCrossTalkSwitch) {
	       //if(ir >= 4 && ir <= 7) ir = getCrossTalkRSegment(ir); // Cross talk implemented for outer sector
               ir = getCrossTalkRSegment(ir);
         }
         */

         // LOG_INFO << "ir2 = " << ir << endm;
         // Phi number
         int ip = int(MAXPHI * pp / 2.0 / PI);
         std::cout << "DISK: " << d-1 << std::endl;
         std::cout << "Phi strip: " << ip << std::endl;

         // Guard against out-of-bounds on constant eta binning
         if (mConstEta == true) {
               if (eta < etaMn)
                  continue;
               if (eta > etaMx)
                  continue;
         }

         // Strip numbers on rastered value (eta)
         int ieta = int(nbins * (eta - etaMn) / (etaMx - etaMn));

         if (ir >= 8 || ir < 0){
               //std::cout << "                                OUT OF BOUNDS" << std::endl;
               continue;
         }  
       
         if (MAXR)
               assert(ir < MAXR);
         if (MAXPHI)
               assert(ip < MAXPHI);

         StRnDHit *fsihit = 0;
         if (_map[d - 1][ir][ip] == 0) // New hit
         {

               if (verbose)
                //  LOG_INFO << Form("NEW d=%1d xyz=%8.4f %8.4f %8.4f r=%8.4f phi=%8.4f iR=%2d iPhi=%4d dE=%8.4f[MeV] truth=%d",
              //                      d, x, y, z, r, p, ir, ip, e * 1000.0, t)
                //           << endm;

               count++;
               fsihit = new StRnDHit();
               fsihit->setDetectorId(kFtsId);
               fsihit->setLayer(d);

               //
               // Set position and position error based on radius-constant bins
               //
               float p0 = (ip + 0.5) * 2.0 * PI / float(MAXPHI);
               float dp = 2.0 * PI / float(MAXPHI) / SQRT12;
               if (false == mConstEta) {
                  // float r0 = RMIN[d - 1] + (ir + 0.5) * (RMAX[d - 1] - RMIN[d - 1]) / float(MAXR);
                  // float dr = (RMAX[d - 1] - RMIN[d - 1]) / float(MAXR);
                  // ONLY valide for the disk array 456, no difference for each disk
                  
		  float r0 = (RSegment[ir] + RSegment[ir + 1]) * 0.5;
                  float dr = RSegment[ir + 1] - RSegment[ir];
                  // LOG_INFO << "r0 = " << r00 << endm;
                  float x0 = r0 * cos(p0) + xc;
                  float y0 = r0 * sin(p0) + yc;
                  assert(TMath::Abs(x0) + TMath::Abs(y0) > 0);
                  float dz = 0.03 / SQRT12;
                  float er = dr / SQRT12;
                  fsihit->setPosition(StThreeVectorF(x0, y0, z));
                  // fsihit->setPositionError(StThreeVectorF(er, dp, dz));
                  fsihit->setPositionError(StThreeVectorF(er, dp, dz));

                  // StThreeVectorF posErr = fsihit->positionError();
                  // cout << " input : " << er*2 <<" , "<<  dp<< " , " << dz <<endl;
                  // cout << " output : " << posErr.x() <<" , "<< posErr.y() << " , " << posErr.z()  <<endl;
               }
               //
               // Set position and error based on eta-constant bins
               //
               else {

                  float thMn = 2.0 * TMath::ATan(TMath::Exp(-(etaMn + (ieta + 1) * deta)));
                  float thMx = 2.0 * TMath::ATan(TMath::Exp(-(etaMn + (ieta + 0) * deta)));
                  float rrMn = z * TMath::Tan(thMn); // rastered r-min
                  float rrMx = z * TMath::Tan(thMx); // rastered r-max

                  if (rr < rrMn || rr > rrMx) {
                     LOG_INFO << Form("NEW d=%1d xyz=%8.4f %8.4f %8.4f r=%8.4f rr=%8.4f phi=%8.4f eta=%8.4f iR=%2d iPhi=%4d iEta=%2d dE=%8.4f[MeV] truth=%d", d, x, y, z, r, rr, p, eta, ir, ip, ieta, e * 1000.0, t) << endm;
                     LOG_INFO << "rrMn = " << rrMn << endm;
                     LOG_INFO << "rrMx = " << rrMx << endm;
                     assert(0);
                  };

                  float r0 = (rrMn + rrMx) * 0.5; // rastered position
                  float x0 = r0 * cos(p0) + xc;   // deraster the position
                  float y0 = r0 * sin(p0) + yc;
                  float dz = 0.2 / SQRT12;
                  float dr = rrMx - rrMn;
                  float er = dr / SQRT12;
                  fsihit->setPosition(StThreeVectorF(x0, y0, z));
                  // fsihit->setPosition(StThreeVectorF(x, y, z));
                  // fsihit->setPositionError(StThreeVectorF(er, dp, dz));
                  fsihit->setPositionError(StThreeVectorF(er, dp, dz));
               }

               fsihit->setErrorMatrix(&Hack1to6(fsihit)[0][0]);

               fsihit->setCharge(e);
               fsihit->setIdTruth(t, 100);
               hits.push_back(fsihit);
               _map[d - 1][ir][ip] = fsihit;
               enrsum[d - 1][ir][ip] += e; // Add energy to running sum
               enrmax[d - 1][ir][ip] = e;  // Set maximum energy (?????)

             //  LOG_INFO << Form("NEW d=%1d xyz=%8.4f %8.4f %8.4f ", d, x, y, z) << endm;
              // LOG_INFO << Form("smeared xyz=%8.4f %8.4f %8.4f ", fsihit->position().x(), fsihit->position().y(), fsihit->position().z()) << endm;

               TVector2 hitpos_mc(x, y);
               TVector2 hitpos_rc(fsihit->position().x(), fsihit->position().y());

               hTrutHitYXDisk->Fill(x, y, d);
               hTrutHitRDisk->Fill(hitpos_mc.Mod(), d);
               if (d == 4)
                  hTrutHitRShower[0]->Fill(hitpos_mc.Mod(), isShower);
               if (d == 5)
                  hTrutHitRShower[1]->Fill(hitpos_mc.Mod(), isShower);
               if (d == 6)
                  hTrutHitRShower[2]->Fill(hitpos_mc.Mod(), isShower);
               hTrutHitPhiDisk->Fill(hitpos_mc.Phi() * 180.0 / TMath::Pi(), d);
               hTrutHitPhiZ->Fill(hitpos_mc.Phi() * 180.0 / TMath::Pi(), z);
               hRecoHitYXDisk->Fill(fsihit->position().x(), fsihit->position().y(), d);
               hRecoHitRDisk->Fill(hitpos_rc.Mod(), d);
               hRecoHitPhiDisk->Fill(hitpos_rc.Phi() * 180.0 / TMath::Pi(), d);
               hRecoHitPhiZ->Fill(hitpos_rc.Phi() * 180.0 / TMath::Pi(), z);
               hGlobalDRDisk->Fill(hitpos_rc.Mod() - hitpos_mc.Mod(), d);
               hGlobalZ->Fill(fsihit->position().z());

	       hRRcVsRMc[ir_mc]->Fill(hitpos_mc.Mod(), hitpos_rc.Mod());
	       hRMc[ir_mc]->Fill(hitpos_mc.Mod());
	       hRRc[ir_mc]->Fill(hitpos_rc.Mod());

               hGlobalDeltaR->Fill(hitpos_rc.Mod() - hitpos_mc.Mod());
               hGlobalDeltaPhi->Fill(hitpos_rc.Phi() - hitpos_mc.Phi());  
               hGlobalDeltaRAll[ir_mc]->Fill(hitpos_rc.Mod() - hitpos_mc.Mod());
  	       hGlobalDeltaPhiAll[ir_mc]->Fill(hitpos_rc.Phi() - hitpos_mc.Phi());           
               h2LocalXY->Fill(x, y);
               h2LocalSmearedXY->Fill(fsihit->position().x(), fsihit->position().y());
               h2LocalDeltaXY->Fill(fsihit->position().x() - x, fsihit->position().y() - y);
               h3LocalDeltaXYDisk->Fill(fsihit->position().x() - x, fsihit->position().y() - y, d);
               // cout << "CHECK : " << fsihit->position().x()-x << " |||  "<<  fsihit->position().y()-y << endl;
               h2GlobalXY->Fill(x, y);
               h2GlobalSmearedXY->Fill(fsihit->position().x(), fsihit->position().y());
               h3GlobalXYDisk->Fill(x, y, d);
               h3GlobalSmearedXYDisk->Fill(fsihit->position().x(), fsihit->position().y(), d);
               h2GlobalDeltaXY->Fill(fsihit->position().x() - x, fsihit->position().y() - y);
               h3GlobalDeltaXYDisk->Fill(fsihit->position().x() - x, fsihit->position().y() - y, d);

               h3LocalDeltaXYR->Fill(fsihit->position().x() - x, fsihit->position().y() - y, sqrt(pow(fsihit->position().x(), 2) + pow(fsihit->position().y(), 2)));
               // h3GlobalXYR-> Fill(x, y, d);
               // h3GlobalSmearedXYR->Fill(fsihit->position().x(), fsihit->position().y(), d);
               // h3GlobalDeltaXYR-> Fill(fsihit->position().x() - x, fsihit->position().y() - y, d);

         } 
         else // Adding energy to old hit
         {
               //     LOG_INFO << Form("ADD d=%1d xyz=%8.4f %8.4f %8.4f r=%8.4f phi=%8.4f iR=%2d iPhi=%4d dE=%8.4f[MeV] truth=%d",
               //            d,x,y,z,r,p,ir,ip,e*1000.0,t) <<endm;

               fsihit = _map[d - 1][ir][ip];
               fsihit->setCharge(fsihit->charge() + e);

               // Add energy to running sum
               enrsum[d - 1][ir][ip] += e;
               double &E = enrmax[d - 1][ir][ip];
               if (e > E)
                  E = e;

               // keep idtruth but dilute it...
               t = fsihit->idTruth();

               fsihit->setIdTruth(t, 100 * E / enrsum[d - 1][ir][ip]);
         }
      }
   }
   int nfsihit = hits.size();

   // TODO: put back to StarRandom global
   // StarRandom &SiRand = StarRandom::Instance();
   //TRandom *SiRand = new TRandom3();
   //SiRand->SetSeed(0);

   // Loop over hits and digitize
   for (int i = 0; i < nfsihit; i++) {
      //hack! do digitization here, or more realistic smearing
      // TODO : PUT BACK TO SiRand with above
      // double rnd_save = SiRand.flat(0., 1.);
      //double rnd_save = SiRand->Rndm();

      // cout <<"to be saved : " << rnd_save << " , discard prob : "<< mInEff << endl;
      //if (rnd_save > mInEff)
         fsicollection->addHit(hits[i]);
   }
   if (verbose)
      //LOG_INFO << Form("Found %d/%d g2t hits in %d cells, created %d hits with ADC>0", count, nHits, nfsihit, fsicollection->numberOfHits()) << endm;
   //    fsicollection->print(1);
   for (int id = 0; id < NDISC; id++) {
      for (int ir = 0; ir < MAXR; ir++) {
         free(enrsum[id][ir]);
         free(enrmax[id][ir]);
      }
   }
   for (int id = 0; id < NDISC; id++) {
      free(enrmax[id]);
      free(enrsum[id]);
   }
   free(enrsum);
   free(enrmax);
   //
   // delete *_map;
   // delete enrmax;
   // delete enrsum;
}
//

int StSiSlowSimulatorMaker::Finish() {
   fOut->cd();
   hTrutHitYXDisk->Write();
   hTrutHitRDisk->Write();
   hTrutHitRShower[0]->Write();
   hTrutHitRShower[1]->Write();
   hTrutHitRShower[2]->Write();

   hGlobalDeltaR->Write();
   hGlobalDeltaPhi->Write();
   for (int i_rstrip = 0; i_rstrip < 8; ++i_rstrip){
       hGlobalDeltaRAll[i_rstrip]->Write();
       hGlobalDeltaPhiAll[i_rstrip]->Write();
       hRRcVsRMc[i_rstrip]->Write();
       hRMc[i_rstrip]->Write();
       hRRc[i_rstrip]->Write();
   }
   hTrutHitPhiDisk->Write();
   hTrutHitPhiZ->Write();
   hRecoHitYXDisk->Write();
   hRecoHitRDisk->Write();
   hRecoHitPhiDisk->Write();
   hRecoHitPhiZ->Write();
   hGlobalDRDisk->Write();
   hGlobalZ->Write();
   // h2LocalXY->Write();
   // h2LocalSmearedXY->Write();
   // h2LocalDeltaXY->Write();
   // h3LocalDeltaXYDisk->Write();
   h3LocalDeltaXYR->Write();
   // h2GlobalXY->Write();
   // h2GlobalSmearedXY->Write();
   // h3GlobalXYDisk->Write();
   // h3GlobalXYR->Write();
   // h3GlobalSmearedXYDisk->Write();
   // h3GlobalSmearedXYR->Write();
   // h2GlobalDeltaXY->Write();
   // h3GlobalDeltaXYDisk->Write();
   // h3GlobalDeltaXYR->Write();
   fOut->Close();
   return kStOK;
}

long long encodeWire(const int &disk,
                     const float &xGlobal,
                     const float &yGlobal,
                     float &xLocal,
                     float &yLocal,
                     float &xCenter,
                     float &yCenter,
                     float &dxCenter,
                     float &dyCenter,
                     float &xPull,
                     float &yPull) {

    /* fsiref3/4/5 */ const float xmin[] = {6, 6, 6, 6, 6, 6};
    /* fsiref3/4/5 */ const float xmax[] = {45.5, 45.5, 71.4, 71.4, 71.4, 71.4};

    //
    // Map into octant local coordinates
    //

    float phiGlobal = TMath::ATan2(xGlobal, yGlobal);

    while (phiGlobal <= 0.)
        phiGlobal += TMath::TwoPi();
    while (phiGlobal > TMath::TwoPi())
        phiGlobal -= TMath::TwoPi();

    float rGlobal = TMath::Sqrt(xGlobal * xGlobal + yGlobal * yGlobal);

    //
    // Note note note note note -- ioctant may NOT be calculated correctly here
    //   ... octant zero covers [-pi/8,+pi/8] ...
    //
    //   ... so it looks like it is shifted by one half of an octant ...
    //

    int ioctant = 4 * (phiGlobal) / TMath::Pi(); // this is the octant number

    float phiLocal = phiGlobal;

    while (phiLocal > TMath::Pi() / 8.0) {
        phiLocal -= TMath::Pi() / 4.0;
    }
    while (phiLocal <= -TMath::Pi() / 8.0) {
        phiLocal += TMath::Pi() / 4.0;
    }

    xLocal = rGlobal * TMath::Cos(phiLocal);
    yLocal = rGlobal * TMath::Sin(phiLocal);

    //
    // Check that xLocal is w/in the boundaries of the octant
    //
    if (xLocal < xmin[disk - 7])
        return -1;
    if (xLocal > xmax[disk - 7])
        return -2;

    //
    // Map into pad local coordinats and strip number
    //
    int ipad = (xLocal - xmin[disk - 7]) / 6.0;
    float xPad = xLocal - xmin[disk - 7] - 6.0 * float(ipad);

    // Divide the octant into three phi bins... Should be 1, 0, -1
    int iphi = (12.0 * phiLocal) / TMath::Pi();

    // Wire center in X ...
    float xPadLow = 6.0 * ipad + xmin[disk - 7];
    xCenter = xPadLow + 3.0;
    dxCenter = 6.0 / SQRT12;

    // Wire center in Y ...
    float yRowLow = -TMath::Tan(PI / 8) * xPadLow;
    float yRowHi = +TMath::Tan(PI / 8) * xPadLow;
    if (yLocal > yRowHi || yLocal < yRowLow) {
        return -1; // Error condition
    }

    int iwire = (yLocal - yRowLow) / 0.15;
    yCenter = yRowLow + iwire * 0.15 + 0.075;
    dyCenter = 0.15 / SQRT12;

    // Compute the pullz
    xPull = (xLocal - xCenter) / dxCenter;
    yPull = (yLocal - yCenter) / dyCenter;

    xLocal = xCenter;
    yLocal = yCenter;

    // Now rotate back to global coordinates
    float dPhiToGlobal = ioctant * TMath::Pi() / 4;
    if (dPhiToGlobal > 0) {
        float X = xCenter;
        float Y = yCenter;

        xCenter = X * TMath::Cos(dPhiToGlobal) - Y * TMath::Sin(dPhiToGlobal);
        yCenter = X * TMath::Sin(dPhiToGlobal) + Y * TMath::Cos(dPhiToGlobal);
    }

    return 1000000 * disk +
           100000 * ioctant +
           10000 * ipad +
           iwire;
}

long long encodeStrip(const int &disk,
                      const float &xGlobal,
                      const float &yGlobal,
                      float &xLocal,
                      float &yLocal,
                      float &xCenter,
                      float &yCenter,
                      float &dxCenter,
                      float &dyCenter,
                      float &xPull,
                      float &yPull) {

    //  float xLocal, yLocal;

    //
    // min and max xlocal coordinates in each octant
    //
    //fsiref1/2  const float xmin[] = { 7.0,  7.0,  15.0, 15.0,  23.0, 23.0 };
    //fsiref1/2 const float xmax[] = { 37.0, 37.0, 75.0, 75.0, 113.0, 113.0 };

    /* fsiref3/4/5 */ const float xmin[] = {6, 6, 6, 6, 6, 6};
    /* fsiref3/4/5 */ const float xmax[] = {45.5, 45.5, 71.4, 71.4, 71.4, 71.4};

    //
    // Map into octant local coordinates
    //

    float phiGlobal = TMath::ATan2(xGlobal, yGlobal);

    while (phiGlobal <= 0.)
        phiGlobal += TMath::TwoPi();
    while (phiGlobal > TMath::TwoPi())
        phiGlobal -= TMath::TwoPi();

    float rGlobal = TMath::Sqrt(xGlobal * xGlobal + yGlobal * yGlobal);

    int ioctant = 4 * (phiGlobal) / TMath::Pi(); // this is the octant number

    float phiLocal = phiGlobal;

    while (phiLocal > TMath::Pi() / 8.0) {
        phiLocal -= TMath::Pi() / 4.0;
    }
    while (phiLocal <= -TMath::Pi() / 8.0) {
        phiLocal += TMath::Pi() / 4.0;
    }

    xLocal = rGlobal * TMath::Cos(phiLocal);
    yLocal = rGlobal * TMath::Sin(phiLocal);

    //
    // Check that xLocal is w/in the boundaries of the octant
    //
    if (xLocal < xmin[disk - 7])
        return -1;
    if (xLocal > xmax[disk - 7])
        return -2;

    //
    // Map into pad local coordinats and strip number
    //
    int ipad = (xLocal - xmin[disk - 7]) / 6.0;
    float xPad = xLocal - xmin[disk - 7] - 6.0 * float(ipad);

    // Divide the octant into three phi bins... Should be 1, 0, -1
    int iphi = (12.0 * phiLocal) / TMath::Pi();
    assert(iphi >= -1);
    assert(iphi <= 1);

    int istrip = (xPad) / 0.3; // 3mm strip size
    float xStrip = istrip * 0.3 + 0.15;

    xCenter = xStrip + ipad * 6.0 + xmin[disk - 7];

    // Calculate width in y at xCenter and the X and Y errors
    float yWidth = 2.0 * xCenter * TMath::Tan(TMath::Pi() / 8.0);

    dxCenter = 0.3 / SQRT12;        // 3mm wide strip
    dyCenter = yWidth / 3 / SQRT12; // 1/3rd width of octant
    //Center = yWidth / 10.3923;

    // Finally, the center of the strip in Y
    float YCENTER[] = {-yWidth / 3, 0, +yWidth / 3};
    yCenter = YCENTER[iphi + 1];

    // LOG_INFO << " xg="      << xGlobal
    //       << " yg="      << yGlobal
    //       << " phig="    << phiGlobal*TMath::RadToDeg()
    //       << " xl="      << xLocal
    //       << " yl="      << yLocal
    //       << " phil="    << phiLocal*TMath::RadToDeg()
    //       << " xpad="    << xPad
    //       << " ioctant=" << ioctant
    //       << " ipad="    << ipad
    //       << " iphi="    << iphi
    //       << " istr="    << istrip
    //       << " xc="      << xCenter
    //       << " yc="      << yCenter
    //       << endm;

    xPull = (xLocal - xCenter) / dxCenter;
    yPull = (yLocal - yCenter) / dyCenter;

    xLocal = xCenter;
    yLocal = yCenter;

    // Now rotate back to global coordinates
    float dPhiToGlobal = ioctant * TMath::Pi() / 4;
    if (dPhiToGlobal > 0) {
        float X = xCenter;
        float Y = yCenter;

        xCenter = X * TMath::Cos(dPhiToGlobal) - Y * TMath::Sin(dPhiToGlobal);
        yCenter = X * TMath::Sin(dPhiToGlobal) + Y * TMath::Cos(dPhiToGlobal);
    }

    // Return a hash code
    return 1000000 * disk + 100000 * ioctant // 8 octants
           + 10000 * iphi                    // 3 phibins
           + 100 * ipad                      // 14 pads
           + 1 * istrip;                     // 20 strips
}

//_____________________________________________________________________________
StMatrixF Hack1to6(const StHit *stHit) {
    //  X = R*cos(Fi), Y=R*sin(Fi), Z = z
    //   dX/dR  = (    cos(Fi)  ,sin(Fi),0)
    //   dX/dFi = (-R*sin(Fi), R*cos(Fi),0)
    //   dX/dZ  = (         0,         0,1)

    auto hiPos = stHit->position();
    auto hiErr = stHit->positionError();
    double Rxy = sqrt(hiPos[0] * hiPos[0] + hiPos[1] * hiPos[1]);
    double cosFi = hiPos[0] / Rxy;
    double sinFi = hiPos[1] / Rxy;
    double T[3][3] = {{cosFi, -Rxy * sinFi, 0}, {sinFi, Rxy * cosFi, 0}, {0, 0, 1}};
    double Ginp[6] = {hiErr[0] * hiErr[0], 0, hiErr[1] * hiErr[1], 0, 0, hiErr[2] * hiErr[2]};
    double Gout[6];

    TCL::trasat(T[0], Ginp, Gout, 3, 3);
    StMatrixF mtxF(3, 3);

    for (int i = 0, li = 0; i < 3; li += ++i) {
        for (int j = 0; j <= i; j++) {
            mtxF[i][j] = Gout[li + j];
            mtxF[j][i] = mtxF[i][j];
        }
    }

    return mtxF;
}

//_____________________________________________________________________________
int StSiSlowSimulatorMaker::getCrossTalkRSegment(int rstrip)
{

  const double ctRate[8][7] = {	
	{0.0000, 0.0000, 0.0000, 0.7419, 0.9413, 0.9775, 1.0000},
	{0.0000, 0.0000, 0.0247, 0.8773, 0.9809, 1.0000, 1.0000},
	{0.0000, 0.0052, 0.0668, 0.8726, 1.0000, 1.0000, 1.0000},
	{0.0097, 0.0651, 0.1495, 1.0000, 1.0000, 1.0000, 1.0000},
	{0.0000, 0.0000, 0.0000, 0.8780, 0.9810, 0.9915, 1.0000},
	{0.0000, 0.0000, 0.1343, 0.9141, 0.9889, 1.0000, 1.0000},
	{0.0000, 0.0372, 0.1889, 0.9288, 1.0000, 1.0000, 1.0000},
	{0.0415, 0.2166, 0.3871, 1.0000, 1.0000, 1.0000, 1.0000}
   };
//print value
  const int deltaBin[7] = {-3, -2, -1, 0, 1, 2, 3};
  
  double ran = gRandom->Uniform(0.0,1.0);

  int ctBin = -999;
  if(rstrip > -100)
  {
    for(int i_delta = 0; i_delta < 7; ++i_delta)
    {
      if(ran >= ctRate[rstrip][i_delta] && ran < ctRate[rstrip][i_delta+1])
      {
	ctBin = i_delta+1;
      }
    }
    if(ran >= 0.0 && ran < ctRate[rstrip][0])
    {
      ctBin = 0;
    }
  }

  if(ctBin > -1) return rstrip + deltaBin[ctBin];

  return ctBin;
}
