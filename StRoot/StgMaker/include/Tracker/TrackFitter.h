#ifndef TRACK_FITTER_H
#define TRACK_FITTER_H

#include "GenFit/ConstField.h"
#include "GenFit/EventDisplay.h"
#include "GenFit/Exception.h"
#include "GenFit/FieldManager.h"
#include "GenFit/KalmanFitStatus.h"
#include "GenFit/KalmanFitter.h"
#include "GenFit/KalmanFitterInfo.h"
#include "GenFit/KalmanFitterRefTrack.h"
#include "GenFit/MaterialEffects.h"
#include "GenFit/PlanarMeasurement.h"
#include "GenFit/RKTrackRep.h"
#include "GenFit/SpacepointMeasurement.h"
#include "GenFit/StateOnPlane.h"
#include "GenFit/TGeoMaterialInterface.h"
#include "GenFit/Track.h"
#include "GenFit/TrackPoint.h"

#include "Criteria/SimpleCircle.h"

#include "TDatabasePDG.h"
#include "TGeoManager.h"
#include "TMath.h"
#include "TRandom.h"
#include "TRandom3.h"
#include "TVector3.h"

#include <vector>

#include "StgMaker/XmlConfig/HistoBins.h"
#include "StgMaker/XmlConfig/XmlConfig.h"
#include "StgMaker/include/Tracker/FwdHit.h"
#include "StgMaker/include/Tracker/STARField.h"
#include "StgMaker/include/Tracker/loguru.h"
#include "StgMaker/include/Tracker/FwdGeomUtils.h"

// hack of a global field pointer
genfit::AbsBField *_gField = 0;

class TrackFitter {

  public:
    TrackFitter(jdb::XmlConfig &_cfg) : cfg(_cfg) {
        fTrackRep = 0;
        fTrack = 0;
    }

    void setup(bool make_display = false) {
        LOG_SCOPE_FUNCTION(INFO);

        TGeoManager * gMan = nullptr;

        {
            LOG_SCOPE_F( INFO, "Setup Geometry in GENFIT" );
            // gMan = new TGeoManager("Geometry", "Geane geometry");
            TGeoManager::Import(cfg.get<string>("Geometry", "fGeom.root").c_str());
            gMan = gGeoManager;
            genfit::MaterialEffects::getInstance()->init(new genfit::TGeoMaterialInterface());
            genfit::MaterialEffects::getInstance()->setNoEffects(cfg.get<bool>("TrackFitter::noMaterialEffects", false));
            if ( cfg.get<bool>("TrackFitter::noMaterialEffects", false) ){
                LOG_F( WARNING, "MaterialEffects are turned OFF" );
            }
        }

        // TODO : Load the STAR MagField
        genfit::AbsBField *bField = nullptr;

        if (0 == _gField) {
            if (cfg.get<bool>("TrackFitter:constB", false)) {
                bField = new genfit::ConstField(0., 0., 5.);
                LOG_F(INFO, "Using a CONST B FIELD");
            } else {
                bField = new genfit::STARFieldXYZ();
                LOG_F(INFO, "Using STAR B FIELD");
            }
        } else {
            LOG_F(INFO, "Using StarMagField interface");
            bField = _gField;
        }

        genfit::FieldManager::getInstance()->init(bField); // 0.5 T Bz

        makeDisplay = make_display;

        if (make_display)
            display = genfit::EventDisplay::getInstance();

        // init the fitter
        fitter = new genfit::KalmanFitterRefTrack();
        // fitter = new genfit::FwdKalmanFitterRefTrack();
        // fitter = new genfit::KalmanFitter( );
        // fitter = new genfit::GblFitter();

        // MaxFailedHits = -1 is default, no restriction
        fitter->setMaxFailedHits(cfg.get<int>("TrackFitter.KalmanFitterRefTrack:MaxFailedHits", -1));
        fitter->setDebugLvl(cfg.get<int>("TrackFitter.KalmanFitterRefTrack:DebugLvl", 0));
        fitter->setMaxIterations(cfg.get<int>("TrackFitter.KalmanFitterRefTrack:MaxIterations", 4));
        fitter->setMinIterations(cfg.get<int>("TrackFitter.KalmanFitterRefTrack:MinIterations", 0));
        LOG_F(INFO, "getMinIterations = %d", fitter->getMinIterations());
        LOG_F(INFO, "getMaxIterations = %d", fitter->getMaxIterations());
        LOG_F(INFO, "getDeltaPval = %f", fitter->getDeltaPval());
        LOG_F(INFO, "getRelChi2Change = %f", fitter->getRelChi2Change());
        LOG_F(INFO, "getBlowUpMaxVal = %f", fitter->getBlowUpMaxVal());
        LOG_F(INFO, "getMaxFailedHits = %d", fitter->getMaxFailedHits());

        // make a super simple version of the FTS geometry detector planes (this needs to be updated)

        // float SI_DET_Z[] = {138.87, 152.87, 166.87};
        LOG_F( INFO, "Setting up the FwdGeomUtils" );
        FwdGeomUtils fwdGeoUtils( gMan );

        LOG_F( INFO, "Setting up Si planes" );
        vector<float> SI_DET_Z = cfg.getFloatVector("TrackFitter.Geometry:si");
        if (SI_DET_Z.size() < 3) {
        
            // try to read from GEOMETRY
            if ( fwdGeoUtils.siZ( 0 ) > 1.0 ) { // returns 0.0 on failure
                SI_DET_Z.clear();
                SI_DET_Z.push_back( fwdGeoUtils.siZ( 0 ) );
                SI_DET_Z.push_back( fwdGeoUtils.siZ( 1 ) );
                SI_DET_Z.push_back( fwdGeoUtils.siZ( 2 ) );
                LOG_F( INFO, "From GEOMETRY : Si Z = %0.2f, %0.2f, %0.2f", SI_DET_Z[0], SI_DET_Z[1], SI_DET_Z[2] );
            } else {
                LOG_F(WARNING, "Using Default Si z locations - that means FTSM NOT in Geometry");
                SI_DET_Z.push_back(140.286011);
                SI_DET_Z.push_back(154.286011);
                SI_DET_Z.push_back(168.286011);
            }
        } else {
            LOG_F( WARNING, "Using Si Z location from config - may not match real geometry" );
        }

        for (auto z : SI_DET_Z) {
            LOG_F(INFO, "Adding Si Detector Plane at (0, 0, %0.2f)", z);
            SiDetPlanes.push_back(genfit::SharedPlanePtr(new genfit::DetPlane(TVector3(0, 0, z), TVector3(1, 0, 0), TVector3(0, 1, 0))));
        }
        useSi = false;

        // Now load STGC
        vector<float> DET_Z = cfg.getFloatVector("TrackFitter.Geometry:stgc");
        if (DET_Z.size() < 4) {
            // try to read from GEOMETRY
            if ( fwdGeoUtils.stgcZ( 0 ) > 1.0 ) { // returns 0.0 on failure
                DET_Z.clear();
                DET_Z.push_back( fwdGeoUtils.stgcZ( 0 ) );
                DET_Z.push_back( fwdGeoUtils.stgcZ( 1 ) );
                DET_Z.push_back( fwdGeoUtils.stgcZ( 2 ) );
                DET_Z.push_back( fwdGeoUtils.stgcZ( 3 ) );
                LOG_F( INFO, "From GEOMETRY : sTGC Z = %0.2f, %0.2f, %0.2f, %0.2f", DET_Z[0], DET_Z[1], DET_Z[2], DET_Z[3] );
            } else {
                DET_Z.push_back(280.904449);
                DET_Z.push_back(303.695099);
                DET_Z.push_back(326.597626);
                DET_Z.push_back(349.400482);
                LOG_F(WARNING, "Using Default STGC z locations");
            }
        } else {
            LOG_F( WARNING, "Using STGC Z location from config - may not match real geometry" );
        }

        for (auto z : DET_Z) {
            LOG_F(INFO, "Adding DetPlane at (0, 0, %0.2f)", z);
            DetPlanes.push_back(genfit::SharedPlanePtr(new genfit::DetPlane(TVector3(0, 0, z), TVector3(1, 0, 0), TVector3(0, 1, 0))));
        }

        // get cfg values
        vertexSigmaXY = cfg.get<float>("TrackFitter.Vertex:sigmaXY", 1);
        vertexSigmaZ = cfg.get<float>("TrackFitter.Vertex:sigmaZ", 30);
        vertexPos = cfg.getFloatVector("TrackFitter.Vertex:pos", 0, 3);
        includeVertexInFit = cfg.get<bool>("TrackFitter.Vertex:includeInFit", false);

        LOG_F(INFO, "vertex pos = (%f, %f, %f)", vertexPos[0], vertexPos[1], vertexPos[2]);
        LOG_F(INFO, "vertex sigma = (%f, %f, %f)", vertexSigmaXY, vertexSigmaXY, vertexSigmaZ);
        LOG_F(INFO, "Include vertex in fit %d", (int)includeVertexInFit);

        rand = new TRandom3();
        rand->SetSeed(0);


        skipSi0 = cfg.get<bool>("TrackFitter.Hits:skipSi0", false);
        skipSi1 = cfg.get<bool>("TrackFitter.Hits:skipSi1", false);

        if (skipSi0) {
            LOG_F(INFO, "Skip Si disk 0 : true");
        }

        if (skipSi1) {
            LOG_F(INFO, "Skip Si disk 1 : true");
        }

        makeHistograms();
    }

    void setBinLabels(TH1 *h, vector<string> labels) {
        for (int i = 1; i < labels.size(); i++) {
            h->GetXaxis()->SetBinLabel(i, labels[i - 1].c_str());
            h->SetBinContent(i, 0);
        }
    }

    void makeHistograms() {
        std::string n = "";
        hist["ECalProjPosXY"] = new TH2F("ECalProjPosXY", ";X;Y", 1000, -500, 500, 1000, -500, 500);
        hist["ECalProjSigmaXY"] = new TH2F("ECalProjSigmaXY", ";#sigma_{X};#sigma_{Y}", 50, 0, 0.5, 50, 0, 0.5);
        hist["ECalProjSigmaR"] = new TH1F("ECalProjSigmaR", ";#sigma_{XY} (cm) at ECAL", 50, 0, 0.5);

        hist["SiProjPosXY"] = new TH2F("SiProjPosXY", ";X;Y", 1000, -500, 500, 1000, -500, 500);
        hist["SiProjSigmaXY"] = new TH2F("SiProjSigmaXY", ";#sigma_{X};#sigma_{Y}", 150, 0, 15, 150, 0, 15);

        hist["VertexProjPosXY"] = new TH2F("VertexProjPosXY", ";X;Y", 100, -5, 5, 100, -5, 5);
        hist["VertexProjSigmaXY"] = new TH2F("VertexProjSigmaXY", ";#sigma_{X};#sigma_{Y}", 150, 0, 20, 150, 0, 20);

        hist["VertexProjPosZ"] = new TH1F("VertexProjPosZ", ";Z;", 100, -50, 50);
        hist["VertexProjSigmaZ"] = new TH1F("VertexProjSigmaZ", ";#sigma_{Z};", 100, 0, 10);

        hist["SiWrongProjPosXY"] = new TH2F("SiWrongProjPosXY", ";X;Y", 1000, -500, 500, 1000, -500, 500);
        hist["SiWrongProjSigmaXY"] = new TH2F("SiWrongProjSigmaXY", ";#sigma_{X};#sigma_{Y}", 50, 0, 0.5, 50, 0, 0.5);

        hist["SiDeltaProjPosXY"] = new TH2F("SiDeltaProjPosXY", ";X;Y", 1000, 0, 20, 1000, 0, 20);

        n = "seed_curv";
        hist[n] = new TH1F(n.c_str(), ";curv", 1000, 0, 10000);
        n = "seed_pT";
        hist[n] = new TH1F(n.c_str(), ";pT (GeV/c)", 500, 0, 10);
        n = "seed_eta";
        hist[n] = new TH1F(n.c_str(), ";eta", 500, 0, 5);

        n = "delta_fit_seed_pT";
        hist[n] = new TH1F(n.c_str(), ";#Delta( fit, seed ) pT (GeV/c)", 500, -5, 5);
        n = "delta_fit_seed_eta";
        hist[n] = new TH1F(n.c_str(), ";#Delta( fit, seed ) eta", 500, 0, 5);
        n = "delta_fit_seed_phi";
        hist[n] = new TH1F(n.c_str(), ";#Delta( fit, seed ) phi", 500, -5, 5);

        n = "FitStatus";
        hist[n] = new TH1F(n.c_str(), ";", 5, 0, 5);
        jdb::HistoBins::labelAxis(hist[n]->GetXaxis(), {"Total", "Pass", "Fail", "GoodCardinal", "Exception"});

        n = "FitDuration";
        hist[n] = new TH1F(n.c_str(), "; Duraton (ms)", 5000, 0, 50000);

        n = "FailedFitDuration";
        hist[n] = new TH1F(n.c_str(), "; Duraton (ms)", 500, 0, 50000);
    }

    void writeHistograms() {
        for (auto nh : hist) {
            nh.second->SetDirectory(gDirectory);
            nh.second->Write();
        }
    }

    /* Convert the 3x3 covmat to 2x2 by dropping z
    *
    */
    TMatrixDSym CovMatPlane(KiTrack::IHit *h){
        TMatrixDSym cm(2);
        cm(0, 0) = static_cast<FwdHit*>(h)->_covmat(0, 0);
        cm(1, 1) = static_cast<FwdHit*>(h)->_covmat(1, 1);
        cm(0, 1) = static_cast<FwdHit*>(h)->_covmat(0, 1);
        return cm;
    }

    /* Get the Covariance Matrix for the detector hit point
	 *
	 */
    TMatrixDSym getCovMat(KiTrack::IHit *hit) {
        // we can calculate the CovMat since we know the det info, but in future we should probably keep this info in the hit itself

        // for sTGC it is simple to compute the cartesian cov mat, do that first.

        // if (hit->getSector() > 2) { // 0-2 are Si, 3-6 are sTGC
        if ( true ) {
            // only need the 2x2 since these are "measurements on a plane"
            TMatrixDSym cm(2);
            const float sigmaXY = 100 * 1e-4; // 100 microns convert to cm
            cm(0, 0) = sigmaXY * sigmaXY;
            cm(1, 1) = sigmaXY * sigmaXY;
            return cm;
        } else {
            // measurements on a plane only need 2x2
            // for Si geom we need to convert from cyl to car coords
            TMatrixDSym cm(2);
            TMatrixD T(2, 2);
            const float x = hit->getX();
            const float y = hit->getY();
            const float R = sqrt(x * x + y * y);
            const float cosphi = x / R;
            const float sinphi = y / R;
            const float sqrt12 = sqrt(12.);
            const float r_size = 3.0; // cm
            const float dr = r_size / sqrt12;
            const float nPhiDivs = 128 * 12; // may change with geom?
            const float dphi = (TMath::TwoPi() / (nPhiDivs)) / sqrt12;

            T(0, 0) = cosphi;
            T(0, 1) = -R * sinphi;
            T(1, 0) = sinphi;
            T(1, 1) = R * cosphi;

            TMatrixD cmcyl(2, 2);
            cmcyl(0, 0) = dr * dr;
            cmcyl(1, 1) = dphi * dphi;

            TMatrixD r = T * cmcyl * T.T();
            const float sigmaX = sqrt(r(0, 0));
            const float sigmaY = sqrt(r(1, 1));
            cm(0, 0) = r(0, 0);
            cm(1, 1) = r(1, 1);
            cm(0, 1) = r(0, 1);
            cm(1, 0) = r(1, 0);

            return cm;
        }

        // ERROR
        LOG_F(ERROR, "cannot compute cov matrix for hit");
        return TMatrixDSym(2);
    }

    /* Get the Covariance Matrix for the detector hit point
	 *
	 */
    TMatrixDSym makeSiCovMat(KiTrack::IHit *hit) {
        // we can calculate the CovMat since we know the det info, but in future we should probably keep this info in the hit itself

        float r_size = cfg.get<float>("SiRasterizer:r", 3.0);
        float phi_size = cfg.get<float>("SiRasterizer:phi", 0.004);

        // measurements on a plane only need 2x2
        // for Si geom we need to convert from cylindrical to cartesian coords
        TMatrixDSym cm(2);
        TMatrixD T(2, 2);
        TMatrixD J(2, 2);
        const float x = hit->getX();
        const float y = hit->getY();
        const float R = sqrt(x * x + y * y);
        const float cosphi = x / R;
        const float sinphi = y / R;
        const float sqrt12 = sqrt(12.);

        const float dr = r_size / sqrt12;
        const float nPhiDivs = 128 * 12; // may change with geom?
        const float dphi = (phi_size) / sqrt12;

        // Setup the Transposed and normal Jacobian transform matrix;
        // note, the si fast sim did this wrong
        // row col
        T(0, 0) = cosphi;
        T(0, 1) = -R * sinphi;
        T(1, 0) = sinphi;
        T(1, 1) = R * cosphi;

        J(0, 0) = cosphi;
        J(0, 1) = sinphi;
        J(1, 0) = -R * sinphi;
        J(1, 1) = R * cosphi;

        TMatrixD cmcyl(2, 2);
        cmcyl(0, 0) = dr * dr;
        cmcyl(1, 1) = dphi * dphi;

        TMatrixD r = T * cmcyl * J;

        const float sigmaX = sqrt(r(0, 0));
        const float sigmaY = sqrt(r(1, 1));

        cm(0, 0) = r(0, 0);
        cm(1, 1) = r(1, 1);
        cm(0, 1) = r(0, 1);
        cm(1, 0) = r(1, 0);

        return cm;
    }

    float fitSimpleCircle(vector<KiTrack::IHit *> trackCand, size_t i0, size_t i1, size_t i2) {
        float curv = 0;

        if (i0 > 12 || i1 > 12 || i2 > 12)
            return 0;

        try {
            KiTrack::SimpleCircle sc(trackCand[i0]->getX(), trackCand[i0]->getY(), trackCand[i1]->getX(), trackCand[i1]->getY(), trackCand[i2]->getX(), trackCand[i2]->getY());
            curv = sc.getRadius();
        } catch (KiTrack::InvalidParameter &e) {
            LOG_F(ERROR, "Cannot get pT seed, only had %lu points", trackCand.size());
        }

        if (isinf(curv))
            curv = 999999.9;

        LOG_F(INFO, "SimpleCircle Fit to (%lu, %lu, %lu) = %f", i0, i1, i2, curv);
        return curv;
    }

    float seedState(vector<KiTrack::IHit *> trackCand, TVector3 &seedPos, TVector3 &seedMom) {
        LOG_SCOPE_FUNCTION(INFO);

        // we require at least 4 hits,  so this should be gauranteed
        assert(trackCand.size() > 3);

        TVector3 p0, p1, p2;

        // we want to use the LAST 3 hits, since silicon doesnt have R information
        // OK for now, since sTGC is 100% efficient so we can assume
        FwdHit *hit_closest_to_IP = static_cast<FwdHit *>(trackCand[0]);

        std::map<size_t, size_t> vol_map; // maps from <key=vol_id> to <value=index in trackCand>

        // init the map
        for (size_t i = 0; i < 13; i++)
            vol_map[i] = -1;

        for (size_t i = 0; i < trackCand.size(); i++) {
            auto fwdHit = static_cast<FwdHit *>(trackCand[i]);
            vol_map[abs(fwdHit->_vid)] = i;
            LOG_F(INFO, "HIT _vid=%d (%f, %f, %f)", fwdHit->_vid, fwdHit->getX(), fwdHit->getY(), fwdHit->getZ());

            // find the hit closest to IP for the initial position seed
            if (hit_closest_to_IP->getZ() > fwdHit->getZ())
                hit_closest_to_IP = fwdHit;
        }

        // enumerate the partitions
        // 12 11 10
        // 12 11 9
        // 12 10 9
        // 11 10 9
        vector<float> curvs;
        curvs.push_back(fitSimpleCircle(trackCand, vol_map[12], vol_map[11], vol_map[10]));
        curvs.push_back(fitSimpleCircle(trackCand, vol_map[12], vol_map[11], vol_map[9]));
        curvs.push_back(fitSimpleCircle(trackCand, vol_map[12], vol_map[10], vol_map[9]));
        curvs.push_back(fitSimpleCircle(trackCand, vol_map[11], vol_map[10], vol_map[9]));

        // average them and exclude failed fits
        float mcurv = 0;
        float nmeas = 0;

        for (size_t i = 0; i < curvs.size(); i++) {
            LOG_F(INFO, "curv[%lu] = %f", i, curvs[i]);

            if (MAKE_HIST)
                this->hist["seed_curv"]->Fill(curvs[i]);

            if (curvs[i] > 10) {
                mcurv += curvs[i];
                nmeas += 1.0;
            }
        }

        if (nmeas >= 1)
            mcurv = mcurv / nmeas;
        else
            mcurv = 10;

        // if ( mcurv < 150 )
        // 	mcurv = 150;

        if (vol_map[9] < 13)
            p0.SetXYZ(trackCand[vol_map[9]]->getX(), trackCand[vol_map[9]]->getY(), trackCand[vol_map[9]]->getZ());

        if (vol_map[10] < 13)
            p1.SetXYZ(trackCand[vol_map[10]]->getX(), trackCand[vol_map[10]]->getY(), trackCand[vol_map[10]]->getZ());

        if (p0.Mag() < 0.00001 || p1.Mag() < 0.00001) {
            LOG_F(ERROR, "bad input hits");
        }

        const double K = 0.00029979; //K depends on the used units
        double pt = mcurv * K * 5;
        double dx = (p1.X() - p0.X());
        double dy = (p1.Y() - p0.Y());
        double dz = (p1.Z() - p0.Z());
        double phi = TMath::ATan2(dy, dx);
        double Rxy = sqrt(dx * dx + dy * dy);
        double theta = TMath::ATan2(Rxy, dz);
        // double eta = -log( tantheta / 2.0 );
        //  probably not the best starting condition, but lets try it

        seedMom.SetPtThetaPhi(pt, theta, phi);
        seedPos.SetXYZ(hit_closest_to_IP->getX(), hit_closest_to_IP->getY(), hit_closest_to_IP->getZ());

        if (MAKE_HIST) {
            this->hist["seed_pT"]->Fill(seedMom.Pt());
            this->hist["seed_eta"]->Fill(seedMom.Eta());
        }

        return mcurv;
    }

    genfit::MeasuredStateOnPlane projectTo(size_t si_plane, genfit::Track *fitTrack) {
        LOG_SCOPE_FUNCTION(INFO);
        if (si_plane > 2) {
            LOG_F(ERROR, "si_plane out of bounds = %lu", si_plane);
            genfit::MeasuredStateOnPlane nil;
            return nil;
        }

        // start with last Si plane closest to STGC
        LOG_F(INFO, "Projecting to Si Plane %lu", si_plane);
        auto detSi = SiDetPlanes[si_plane];
        genfit::MeasuredStateOnPlane tst = fitTrack->getFittedState(1);
        auto TCM = fitTrack->getCardinalRep()->get6DCov(tst);
        double len = fitTrack->getCardinalRep()->extrapolateToPlane(tst, detSi, false, true);

        LOG_F(INFO, "len to Si (Disk %lu) =%f", si_plane, len);
        LOG_F(INFO, "Position at Si (%0.2f, %0.2f, %0.2f) +/- (%0.2f, %0.2f, %0.2f)", tst.getPos().X(), tst.getPos().Y(), tst.getPos().Z(), sqrt(TCM(0, 0)), sqrt(TCM(1, 1)), sqrt(TCM(2, 2)));
        TCM = fitTrack->getCardinalRep()->get6DCov(tst);
        return tst;
    }

    TVector3 refitTrackWithSiHits(genfit::Track *originalTrack, std::vector<KiTrack::IHit *> si_hits) {
        LOG_SCOPE_FUNCTION(INFO);

        TVector3 pOrig = originalTrack->getCardinalRep()->getMom(originalTrack->getFittedState(1, originalTrack->getCardinalRep()));
        auto cardinalStatus = originalTrack->getFitStatus(originalTrack->getCardinalRep());

        if (originalTrack->getFitStatus(originalTrack->getCardinalRep())->isFitConverged() == false) {
            LOG_F(WARNING, "Original Track fit did not converge, skipping");
            return pOrig;
        }

        auto trackRepPos = new genfit::RKTrackRep(pdg_mu_plus);
        auto trackRepNeg = new genfit::RKTrackRep(pdg_mu_minus);

        auto trackPoints = originalTrack->getPointsWithMeasurement();
        LOG_F(INFO, "trackPoints.size() = %lu", trackPoints.size());
        if ((trackPoints.size() < 5 && includeVertexInFit) || trackPoints.size() < 4) {
            LOG_F(ERROR, "TrackPoints missing");
            return pOrig;
        }

        TVectorD rawCoords = trackPoints[0]->getRawMeasurement()->getRawHitCoords();
        double z = 280.9; //first Stgc, a hack for now
        if (includeVertexInFit)
            z = rawCoords(2);
        TVector3 seedPos(rawCoords(0), rawCoords(1), z);
        TVector3 seedMom = pOrig;
        LOG_F(INFO, "SeedMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", seedMom.Pt(), seedMom.Eta(), seedMom.Phi());
        LOG_F(INFO, "SeedPos( X=%0.2f, Y=%0.2f, Z=%0.2f )", seedPos.X(), seedPos.Y(), seedPos.Z());

        auto refTrack = new genfit::Track(trackRepPos, seedPos, seedMom);
        refTrack->addTrackRep(trackRepNeg);

        genfit::Track &fitTrack = *refTrack;

        size_t first_tp = 0;
        if (includeVertexInFit) {
            // clone the PRIMARY VERTEX into this track
            fitTrack.insertPoint(new genfit::TrackPoint(trackPoints[0]->getRawMeasurement(), &fitTrack));
            first_tp = 1; // start on the 1st stgc hit below
        }

        // initialize the hit coords on plane
        TVectorD hitCoords(2);
        hitCoords[0] = 0;
        hitCoords[1] = 0;

        int planeId(0);
        int hitId(5);

        bool usingRasteredHits = cfg.get<bool>("SiRasterizer:active", false);

        // add the hits to the track
        for (auto h : si_hits) {
            if ( nullptr == h ) continue; // if no Si hit in this plane, skip

            hitCoords[0] = h->getX();
            hitCoords[1] = h->getY();
            genfit::PlanarMeasurement *measurement = new genfit::PlanarMeasurement(hitCoords, CovMatPlane(h), h->getSector(), ++hitId, nullptr);

            planeId = h->getSector();

            if (SiDetPlanes.size() <= planeId) {
                LOG_F(ERROR, "invalid VolumId -> out of bounds DetPlane, vid = %d", planeId);
                return pOrig;
            }

            auto plane = SiDetPlanes[planeId];
            measurement->setPlane(plane, planeId);
            fitTrack.insertPoint(new genfit::TrackPoint(measurement, &fitTrack));
        }
        LOG_F(INFO, "Vertex plus Si, Track now has %lu points", fitTrack.getNumPoints());

        for (int i = first_tp; i < trackPoints.size(); i++) {
            // clone the track points into this track
            fitTrack.insertPoint(new genfit::TrackPoint(trackPoints[i]->getRawMeasurement(), &fitTrack));
        }
        LOG_F(INFO, "Vertex plus Si plus Stgc, Track now has %lu points", fitTrack.getNumPoints());

        try {
            LOG_SCOPE_F(INFO, "Track RE-Fit with GENFIT2");
            fitTrack.checkConsistency();

            // do the fit
            fitter->processTrack(&fitTrack);

            fitTrack.checkConsistency();
            fitTrack.determineCardinalRep(); // this chooses the lowest chi2 fit result as cardinal

            {
                // print fit result
                // LOG_SCOPE_F(INFO, "Fitted State AFTER Refit");
                // fitTrack.getFittedState().Print();
            }
        } catch (genfit::Exception &e) {
            LOG_F(INFO, "Exception on track RE-fit : %s", e.what());
        }

        if (fitTrack.getFitStatus(fitTrack.getCardinalRep())->isFitConverged() == false) {
            LOG_F(ERROR, "Fit with Si hits did not converge");
        } else {
            TVector3 p = fitTrack.getCardinalRep()->getMom(fitTrack.getFittedState(1, fitTrack.getCardinalRep()));
            LOG_F(INFO, "FitMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", p.Pt(), p.Eta(), p.Phi());
            LOG_F(INFO, "Original FitMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", pOrig.Pt(), pOrig.Eta(), pOrig.Phi());
            auto newStatus = fitTrack.getFitStatus(fitTrack.getCardinalRep());
            LOG_F(INFO, "Chi2 before / after = %f / %f", cardinalStatus->getChi2(), newStatus->getChi2());

            return p;
        }
        return pOrig;
    }

    void studyProjectionToSi(genfit::AbsTrackRep *cardinalRep, genfit::AbsTrackRep *wrongRep, genfit::Track &fitTrack) {

        // try projecting onto the Si plane
        const float DET_Z = 140;
        auto detSi = genfit::SharedPlanePtr(new genfit::DetPlane(TVector3(0, 0, DET_Z), TVector3(1, 0, 0), TVector3(0, 1, 0)));
        genfit::MeasuredStateOnPlane tst = fitTrack.getFittedState(1);
        genfit::MeasuredStateOnPlane tst2 = fitTrack.getFittedState(1, wrongRep);

        auto TCM = cardinalRep->get6DCov(tst);
        // LOG_F( INFO, "Position pre ex (%0.2f, %0.2f, %0.2f) +/- ()", cardinalRep->getPos(tst).X(), cardinalRep->getPos(tst).Y(), cardinalRep->getPos(tst).Z() );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(0,0), TCM(0,1), TCM(0,2), TCM(0,3), TCM(0,4), TCM(0,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(1,0), TCM(1,1), TCM(1,2), TCM(1,3), TCM(1,4), TCM(1,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(2,0), TCM(2,1), TCM(2,2), TCM(2,3), TCM(2,4), TCM(2,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(3,0), TCM(3,1), TCM(3,2), TCM(3,3), TCM(3,4), TCM(3,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(4,0), TCM(4,1), TCM(4,2), TCM(4,3), TCM(4,4), TCM(4,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(5,0), TCM(5,1), TCM(5,2), TCM(5,3), TCM(5,4), TCM(5,5) );

        double len = cardinalRep->extrapolateToPlane(tst, detSi, false, true);
        double len2 = wrongRep->extrapolateToPlane(tst2, detSi, false, true);

        LOG_F(INFO, "len to Si %f", len);
        LOG_F(INFO, "Position at Si (%0.2f, %0.2f, %0.2f) +/- ()", tst.getPos().X(), tst.getPos().Y(), tst.getPos().Z());
        TCM = cardinalRep->get6DCov(tst);
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(0, 0), TCM(0, 1), TCM(0, 2), TCM(0, 3), TCM(0, 4), TCM(0, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(1, 0), TCM(1, 1), TCM(1, 2), TCM(1, 3), TCM(1, 4), TCM(1, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(2, 0), TCM(2, 1), TCM(2, 2), TCM(2, 3), TCM(2, 4), TCM(2, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(3, 0), TCM(3, 1), TCM(3, 2), TCM(3, 3), TCM(3, 4), TCM(3, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(4, 0), TCM(4, 1), TCM(4, 2), TCM(4, 3), TCM(4, 4), TCM(4, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(5, 0), TCM(5, 1), TCM(5, 2), TCM(5, 3), TCM(5, 4), TCM(5, 5));

        auto TCM2 = wrongRep->get6DCov(tst2);

        if (MAKE_HIST) {

            this->hist["SiProjPosXY"]->Fill(tst.getPos().X(), tst.getPos().Y());
            this->hist["SiProjSigmaXY"]->Fill(sqrt(TCM(0, 0)), sqrt(TCM(1, 1)));

            this->hist["SiWrongProjPosXY"]->Fill(tst2.getPos().X(), tst2.getPos().Y());
            this->hist["SiWrongProjSigmaXY"]->Fill(sqrt(TCM2(0, 0)), sqrt(TCM2(1, 1)));

            LOG_F(INFO, "DeltaX Si Proj = %f", fabs(tst.getPos().X() - tst2.getPos().X()));
            this->hist["SiDeltaProjPosXY"]->Fill(fabs(tst.getPos().X() - tst2.getPos().X()), fabs(tst.getPos().Y() - tst2.getPos().Y()));
        }
    }

    void studyProjectionToVertex(genfit::AbsTrackRep *cardinalRep, genfit::Track &fitTrack) {

        // try projecting onto the Si plane
        genfit::MeasuredStateOnPlane tst = fitTrack.getFittedState(1);

        auto TCM = cardinalRep->get6DCov(tst);

        double len = cardinalRep->extrapolateToLine(tst, TVector3(0, 0, 0), TVector3(0, 0, 1));

        LOG_F(INFO, "len to (0,0) %f", len);
        LOG_F(INFO, "Position at (0,0) (%0.2f, %0.2f, %0.2f) +/- ()", tst.getPos().X(), tst.getPos().Y(), tst.getPos().Z());
        TCM = cardinalRep->get6DCov(tst);
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(0, 0), TCM(0, 1), TCM(0, 2), TCM(0, 3), TCM(0, 4), TCM(0, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(1, 0), TCM(1, 1), TCM(1, 2), TCM(1, 3), TCM(1, 4), TCM(1, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(2, 0), TCM(2, 1), TCM(2, 2), TCM(2, 3), TCM(2, 4), TCM(2, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(3, 0), TCM(3, 1), TCM(3, 2), TCM(3, 3), TCM(3, 4), TCM(3, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(4, 0), TCM(4, 1), TCM(4, 2), TCM(4, 3), TCM(4, 4), TCM(4, 5));
        LOG_F(INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(5, 0), TCM(5, 1), TCM(5, 2), TCM(5, 3), TCM(5, 4), TCM(5, 5));

        if (MAKE_HIST) {

            this->hist["VertexProjPosXY"]->Fill(tst.getPos().X(), tst.getPos().Y());
            this->hist["VertexProjSigmaXY"]->Fill(sqrt(TCM(0, 0)), sqrt(TCM(1, 1)));

            this->hist["VertexProjPosZ"]->Fill(tst.getPos().Z());
            this->hist["VertexProjSigmaZ"]->Fill(sqrt(TCM(2, 2)));

            // this->hist[ "SiWrongProjPosXY" ]->Fill( tst2.getPos().X(), tst2.getPos().Y() );
            // this->hist[ "SiWrongProjSigmaXY" ]->Fill( sqrt(TCM2(0, 0)), sqrt(TCM2(1, 1)) );

            // LOG_F( INFO, "DeltaX Si Proj = %f", fabs( tst.getPos().X() - tst2.getPos().X()) );
            // this->hist[ "SiDeltaProjPosXY" ]->Fill( fabs( tst.getPos().X() - tst2.getPos().X()), fabs( tst.getPos().Y() - tst2.getPos().Y()) );
        }
    }

    void studyProjectionToECal(genfit::AbsTrackRep *cardinalRep, genfit::Track &fitTrack) {

        // try projecting onto the Si plane
        auto detSi = genfit::SharedPlanePtr(new genfit::DetPlane(TVector3(0, 0, 711), TVector3(1, 0, 0), TVector3(0, 1, 0)));
        genfit::MeasuredStateOnPlane tst = fitTrack.getFittedState(1);

        auto TCM = cardinalRep->get6DCov(tst);
        // LOG_F( INFO, "Position pre ex (%0.2f, %0.2f, %0.2f) +/- ()", cardinalRep->getPos(tst).X(), cardinalRep->getPos(tst).Y(), cardinalRep->getPos(tst).Z() );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(0,0), TCM(0,1), TCM(0,2), TCM(0,3), TCM(0,4), TCM(0,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(1,0), TCM(1,1), TCM(1,2), TCM(1,3), TCM(1,4), TCM(1,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(2,0), TCM(2,1), TCM(2,2), TCM(2,3), TCM(2,4), TCM(2,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(3,0), TCM(3,1), TCM(3,2), TCM(3,3), TCM(3,4), TCM(3,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(4,0), TCM(4,1), TCM(4,2), TCM(4,3), TCM(4,4), TCM(4,5) );
        // LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(5,0), TCM(5,1), TCM(5,2), TCM(5,3), TCM(5,4), TCM(5,5) );

        double len = cardinalRep->extrapolateToPlane(tst, detSi, false, true);

        LOG_F(INFO, "len to ECal %f", len);
        LOG_F(INFO, "Position at ECal (%0.2f, %0.2f, %0.2f) +/- ()", tst.getPos().X(), tst.getPos().Y(), tst.getPos().Z());
        TCM = cardinalRep->get6DCov(tst);
        LOG_F(INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(0, 0), TCM(0, 1), TCM(0, 2), TCM(0, 3), TCM(0, 4), TCM(0, 5));
        LOG_F(INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(1, 0), TCM(1, 1), TCM(1, 2), TCM(1, 3), TCM(1, 4), TCM(1, 5));
        LOG_F(INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(2, 0), TCM(2, 1), TCM(2, 2), TCM(2, 3), TCM(2, 4), TCM(2, 5));
        LOG_F(INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(3, 0), TCM(3, 1), TCM(3, 2), TCM(3, 3), TCM(3, 4), TCM(3, 5));
        LOG_F(INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(4, 0), TCM(4, 1), TCM(4, 2), TCM(4, 3), TCM(4, 4), TCM(4, 5));
        LOG_F(INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(5, 0), TCM(5, 1), TCM(5, 2), TCM(5, 3), TCM(5, 4), TCM(5, 5));

        if (MAKE_HIST) {
            this->hist["ECalProjPosXY"]->Fill(tst.getPos().X(), tst.getPos().Y());
            float sigmaR = sqrt(TCM(0, 0) + TCM(1, 1));
            this->hist["ECalProjSigmaR"]->Fill( sigmaR );
            this->hist["ECalProjSigmaXY"]->Fill(sqrt(TCM(0, 0)), sqrt(TCM(1, 1)));
        }
    }

    TVector3 fitSpacePoints( vector<genfit::SpacepointMeasurement*> spoints, TVector3 &seedPos, TVector3 &seedMom ){
        LOG_SCOPE_FUNCTION(INFO);
        auto trackRepPos = new genfit::RKTrackRep(pdg_mu_plus);
        auto trackRepNeg = new genfit::RKTrackRep(pdg_mu_minus);

        auto fTrack = new genfit::Track(trackRepPos, seedPos, seedMom);
        fTrack->addTrackRep(trackRepNeg);

        genfit::Track &fitTrack = *fTrack;


        try {
            for ( size_t i = 0; i < spoints.size(); i++ ){
                fitTrack.insertPoint(new genfit::TrackPoint(spoints[i], &fitTrack));
            }

            LOG_SCOPE_F(INFO, "Track Fit with GENFIT2");
            // do the fit
            LOG_F(INFO, "Processing Track");
            fitter->processTrackWithRep(&fitTrack, trackRepPos);
            fitter->processTrackWithRep(&fitTrack, trackRepNeg);

        } catch (genfit::Exception &e) {
            LOG_F(INFO, "%s", e.what());
            LOG_F(INFO, "Exception on track fit");
            std::cerr << e.what();
        }

        try {
        fitTrack.checkConsistency();

        fitTrack.determineCardinalRep();
        auto cardinalRep = fitTrack.getCardinalRep();
        auto cardinalStatus = fitTrack.getFitStatus(cardinalRep);

        TVector3 p = cardinalRep->getMom(fitTrack.getFittedState(1, cardinalRep));
        int _q = cardinalRep->getCharge(fitTrack.getFittedState(1, cardinalRep));

        return p;
        } catch (genfit::Exception &e) {
            LOG_F(INFO, "%s", e.what());
            LOG_F(INFO, "Exception on track fit");
            std::cerr << e.what();
        }
        return TVector3(0, 0, 0);

    }

    TVector3 fitTrack(vector<KiTrack::IHit *> trackCand, double *Vertex = 0, TVector3 *McSeedMom = 0) {
        LOG_SCOPE_FUNCTION(INFO);
        LOG_INFO << "****************************************************" << endm;

        long long itStart = loguru::now_ns();

        LOG_F(INFO, "Track candidate size: %lu", trackCand.size());
        this->hist["FitStatus"]->Fill("Total", 1);

        // The PV information, if we want to use it
        TVectorD pv(3);

        if (0 == Vertex) {
            pv[0] = vertexPos[0] + rand->Gaus(0, vertexSigmaXY);
            pv[1] = vertexPos[1] + rand->Gaus(0, vertexSigmaXY);
            pv[2] = vertexPos[2] + rand->Gaus(0, vertexSigmaZ);
        } else {
            pv[0] = Vertex[0];
            pv[1] = Vertex[1];
            pv[2] = Vertex[2];
        }

        // get the seed info from our hits
        TVector3 seedMom, seedPos;
        float curv = seedState(trackCand, seedPos, seedMom);
        if (seedMom.Pt() < 0.2) {
            TVector3(0, 0, 0);
        }

        // create the track representations
        auto trackRepPos = new genfit::RKTrackRep(pdg_mu_plus);
        auto trackRepNeg = new genfit::RKTrackRep(pdg_mu_minus);

        // If we use the PV, use that as the start pos for the track
        if (includeVertexInFit) {
            seedPos.SetXYZ(pv[0], pv[1], pv[2]);
        }

        if (fTrack)
            delete fTrack;

        if (McSeedMom != nullptr) {
            seedMom = *McSeedMom;
            LOG_F(INFO, "Using MC Seed Momentum (%f, %f, %f)", seedMom.Pt(), seedMom.Eta(), seedMom.Phi());
        }

        fTrack = new genfit::Track(trackRepPos, seedPos, seedMom);
        fTrack->addTrackRep(trackRepNeg);

        LOG_INFO
            << "seedPos : (" << seedPos.X() << ", " << seedPos.Y() << ", " << seedPos.Z() << " )"
            << ", seedMom : (" << seedMom.X() << ", " << seedMom.Y() << ", " << seedMom.Z() << " )"
            << ", seedMom : (" << seedMom.Pt() << ", " << seedMom.Eta() << ", " << seedMom.Phi() << " )"
            << endm;

        genfit::Track &fitTrack = *fTrack;

        const int detId(0); // detector ID
        int planeId(0);     // detector plane ID
        int hitId(0);       // hit ID

        // initialize the hit coords on plane
        TVectorD hitCoords(2);
        hitCoords[0] = 0;
        hitCoords[1] = 0;

        /******************************************************************************************************************************
        * Include the Primary vertex if desired
        *******************************************************************************************************************************/
        if (includeVertexInFit) {

            TMatrixDSym hitCov3(3);
            hitCov3(0, 0) = vertexSigmaXY * vertexSigmaXY;
            hitCov3(1, 1) = vertexSigmaXY * vertexSigmaXY;
            hitCov3(2, 2) = vertexSigmaZ * vertexSigmaZ;

            genfit::SpacepointMeasurement *measurement = new genfit::SpacepointMeasurement(pv, hitCov3, 0, ++hitId, nullptr);
            fitTrack.insertPoint(new genfit::TrackPoint(measurement, &fitTrack));
        }

        /******************************************************************************************************************************
		 * loop over the hits, add them to the track
		 *******************************************************************************************************************************/
        for (auto h : trackCand) {

            hitCoords[0] = h->getX();
            hitCoords[1] = h->getY();
            LOG_F(INFO, "PlanarMeasurement( x=%f, y=%f, sector=%d, hitId=%d )", hitCoords[0], hitCoords[1], h->getSector(), hitId + 1);
            genfit::PlanarMeasurement *measurement = new genfit::PlanarMeasurement(hitCoords, CovMatPlane(h), h->getSector(), ++hitId, nullptr);

            planeId = h->getSector();

            if (DetPlanes.size() <= planeId) {
                LOG_F(ERROR, "invalid VolumId -> out of bounds DetPlane, vid = %d", planeId);
                return TVector3(0, 0, 0);
            }

            auto plane = DetPlanes[planeId];
            measurement->setPlane(plane, planeId);
            fitTrack.insertPoint(new genfit::TrackPoint(measurement, &fitTrack));

            if (abs(h->getZ() - plane->getO().Z()) > 0.05) {
                LOG_F(ERROR, "Z Mismatch h->z = %f, plane->z = %f, diff = %f ", h->getZ(), plane->getO().Z(), abs(h->getZ() - plane->getO().Z()));
            }
        }

        LOG_F(INFO, "TRACK has %u hits", fitTrack.getNumPointsWithMeasurement());

        try {
            LOG_SCOPE_F(INFO, "Track Fit with GENFIT2");

            // do the fit
            LOG_F(INFO, "Processing Track");
            fitter->processTrackWithRep(&fitTrack, trackRepPos);
            fitter->processTrackWithRep(&fitTrack, trackRepNeg);
            // fitter->processTrack( &fitTrack );

            // print fit result
            // fitTrack.getFittedState().Print();
        } catch (genfit::Exception &e) {
            LOG_F(INFO, "%s", e.what());
            LOG_F(INFO, "Exception on track fit");
            // std::cerr << e.what();
            // std::cerr << "Exception on track fit" << std::endl;
            hist["FitStatus"]->Fill("Exception", 1);
        }

        LOG_F(INFO, "Get fit status and momentum");
        TVector3 p(0, 0, 0);

        try {
            LOG_SCOPE_F(INFO, "Get Track Fit status and momentum etc.");
            //check
            fitTrack.checkConsistency();

            fitTrack.determineCardinalRep();
            auto cardinalRep = fitTrack.getCardinalRep();
            auto cardinalStatus = fitTrack.getFitStatus(cardinalRep);
            fStatus = *cardinalStatus; // save the status of last fit

            // Delete any previous track rep
            if (fTrackRep)
                delete fTrackRep;

            // Clone the cardinal rep for persistency
            fTrackRep = cardinalRep->clone(); // save the result of the fit
            if (fitTrack.getFitStatus(cardinalRep)->isFitConverged()) {
                this->hist["FitStatus"]->Fill("GoodCardinal", 1);
            }

            if (fitTrack.getFitStatus(trackRepPos)->isFitConverged() == false &&
                fitTrack.getFitStatus(trackRepNeg)->isFitConverged() == false) {
                LOG_F(INFO, "*********************FIT SUMMARY*********************");
                LOG_F(ERROR, "Track Fit failed to converge");
                LOG_F(INFO, "SeedMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", seedMom.Pt(), seedMom.Eta(), seedMom.Phi());
                LOG_F(INFO, "Estimated Curv: %f", curv);
                LOG_F(INFO, "SeedPosALT( X=%0.2f, Y=%0.2f, Z=%0.2f )", seedPos.X(), seedPos.Y(), seedPos.Z());
                p.SetXYZ(0, 0, 0);
                this->hist["FitStatus"]->Fill("Fail", 1);

                long long duration = (loguru::now_ns() - itStart) * 1e-6; // milliseconds
                this->hist["FailedFitDuration"]->Fill(duration);
                return p;
            }

            // fStatus = *cardinalStatus; // save the status of last fit
            p = cardinalRep->getMom(fitTrack.getFittedState(1, cardinalRep));
            _q = cardinalRep->getCharge(fitTrack.getFittedState(1, cardinalRep));
            _p = p;

        } catch (genfit::Exception &e) {
            LOG_F(INFO, "*********************FIT SUMMARY*********************");
            std::cerr << e.what();
            std::cerr << "Exception on track fit" << std::endl;
            LOG_F(ERROR, "Track Fit failed");
            LOG_F(INFO, "SeedMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", seedMom.Pt(), seedMom.Eta(), seedMom.Phi());
            LOG_F(INFO, "Estimated Curv: %f", curv);
            LOG_F(INFO, "SeedPosALT( X=%0.2f, Y=%0.2f, Z=%0.2f )", seedPos.X(), seedPos.Y(), seedPos.Z());
            p.SetXYZ(0, 0, 0);
            this->hist["FitStatus"]->Fill("Exception", 1);

            long long duration = (loguru::now_ns() - itStart) * 1e-6; // milliseconds
            this->hist["FailedFitDuration"]->Fill(duration);

            return p;
        }

        if (makeDisplay) {
            displayTracks.push_back(fitTrack);
            // event.push_back(&fitTrack);
            display->addEvent(&(displayTracks[displayTracks.size() - 1]));
            // display->setOptions("ABDEFHMPT"); // G show geometry
        }

        LOG_F(INFO, "*********************FIT SUMMARY*********************");
        LOG_F(INFO, "SeedMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", seedMom.Pt(), seedMom.Eta(), seedMom.Phi());
        LOG_F(INFO, "Estimated Curv: %f", curv);
        LOG_F(INFO, "SeedPosALT( X=%0.2f, Y=%0.2f, Z=%0.2f )", seedPos.X(), seedPos.Y(), seedPos.Z());
        LOG_F(INFO, "FitMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", p.Pt(), p.Eta(), p.Phi());
        hist["FitStatus"]->Fill("Pass", 1);

        if (MAKE_HIST) {
            this->hist["delta_fit_seed_pT"]->Fill(p.Pt() - seedMom.Pt());
            this->hist["delta_fit_seed_eta"]->Fill(p.Eta() - seedMom.Eta());
            this->hist["delta_fit_seed_phi"]->Fill(p.Phi() - seedMom.Phi());
        }

        long long duration = (loguru::now_ns() - itStart) * 1e-6; // milliseconds
        this->hist["FitDuration"]->Fill(duration);

        return p;
    }

    void showEvents() {
        if (makeDisplay) {
            display->setOptions("ABDEFGHMPT"); // G show geometry
            display->open();
        }
    }

    int getCharge() {
        return (int)_q;
    }

    genfit::FitStatus getStatus() { return fStatus; }
    genfit::AbsTrackRep *getTrackRep() { return fTrackRep; }
    genfit::Track *getTrack() { return fTrack; }

  private:
    jdb::XmlConfig &cfg;
    std::map<std::string, TH1 *> hist;
    bool MAKE_HIST = true;
    genfit::EventDisplay *display;
    std::vector<genfit::Track *> event;
    vector<genfit::Track> displayTracks;
    bool makeDisplay = false;
    genfit::AbsKalmanFitter *fitter = nullptr;

    const int pdg_pi_plus = 211;
    const int pdg_pi_minus = -211;
    const int pdg_mu_plus = 13;
    const int pdg_mu_minus = -13;

    genfit::AbsTrackRep *pion_track_rep = nullptr;
    vector<genfit::SharedPlanePtr> DetPlanes;
    vector<genfit::SharedPlanePtr> SiDetPlanes;

    TRandom *rand = nullptr;

    // parameter ALIASED from cfg
    float vertexSigmaXY = 1;
    float vertexSigmaZ = 30;
    vector<float> vertexPos;
    bool includeVertexInFit = false;
    bool useSi = true;
    bool skipSi0 = false;
    bool skipSi1 = false;

    genfit::FitStatus fStatus;
    genfit::AbsTrackRep *fTrackRep;
    genfit::Track *fTrack;

    // Fit results
    TVector3 _p;
    double _q;
};

#endif
