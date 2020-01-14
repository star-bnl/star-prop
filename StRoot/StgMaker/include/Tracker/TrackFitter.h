#ifndef TRACK_FITTER_H
#define TRACK_FITTER_H

#include "GenFit/ConstField.h"
#include "GenFit/Exception.h"
#include "GenFit/FieldManager.h"
#include "GenFit/KalmanFitterRefTrack.h"
#include "GenFit/KalmanFitter.h"
#include "GenFit/StateOnPlane.h"
#include "GenFit/Track.h"
#include "GenFit/TrackPoint.h"
#include "GenFit/MaterialEffects.h"
#include "GenFit/RKTrackRep.h"
#include "GenFit/TGeoMaterialInterface.h"
#include "GenFit/EventDisplay.h"
#include "GenFit/PlanarMeasurement.h"
#include "GenFit/SpacepointMeasurement.h"

#include "Criteria/SimpleCircle.h"

#include "TGeoManager.h"
#include "TRandom.h"
#include "TRandom3.h"
#include "TVector3.h"
#include "TDatabasePDG.h"
#include "TMath.h"

#include <vector>

#include "StgMaker/include/Tracker/FwdHit.h"
#include "StgMaker/include/Tracker/STARField.h"
#include "StgMaker/include/Tracker/loguru.h"
#include "StgMaker/XmlConfig/XmlConfig.h"


// hack of a global field pointer
genfit::AbsBField *_gField = 0;

class TrackFitter
{

public:
	TrackFitter( jdb::XmlConfig &_cfg ) : cfg(_cfg)
	{
		fTrackRep = 0;
		fTrack    = 0;
	}

	void setup( bool make_display = false )
	{
		new TGeoManager("Geometry", "Geane geometry");
		TGeoManager::Import( cfg.get<string>( "Geometry", "fGeom.root" ).c_str() );
		genfit::MaterialEffects::getInstance()->init(new genfit::TGeoMaterialInterface());
		//		genfit::MaterialEffects::getInstance()->setDebugLvl(10); // setNoEffects(); // TEST
		//		genfit::MaterialEffects::getInstance()->setNoEffects(); // TEST
		// TODO : Load the STAR MagField
		genfit::AbsBField *bField = nullptr;

		if ( 0 == _gField ) {
			if ( cfg.get<bool>( "TrackFitter:constB", false ) ) {
				bField = new genfit::ConstField(0., 0., 5.);
				LOG_F( INFO, "Using a CONST B FIELD" );
			}
			else {
				bField = new genfit::STARFieldXYZ();
				LOG_F( INFO, "Using STAR B FIELD" );
			}
		}
		else {
			LOG_F( INFO, "Using StarMagField interface" );
			bField = _gField;
		}

		genfit::FieldManager::getInstance()->init( bField ); // 0.5 T Bz

		makeDisplay = make_display;

		if ( make_display )
			display = genfit::EventDisplay::getInstance();

		// init the fitter
		fitter = new genfit::KalmanFitterRefTrack( );
		//	fitter = new genfit::KalmanFitter( );
		fitter->setMaxIterations(2);
		//		fitter->setDebugLvl(10);
		// track representation
		// pion_track_rep =


		// make a super simple version of the FTS geometry detector planes (this needs to be updated)



		float SI_DET_Z[] = {138.87, 152.87, 166.87 };

		for ( auto z : SI_DET_Z ) {
			LOG_F( INFO, "Adding Si Detector Plane at (0, 0, %0.2f)", z );
			SiDetPlanes.push_back( genfit::SharedPlanePtr(new genfit::DetPlane(TVector3(0, 0, z), TVector3(1, 0, 0), TVector3(0, 1, 0))) );
		}
		
		useSi = false;
		float DET_Z[] = {281, 304, 327, 349 };

		for ( auto z : DET_Z ) {
			LOG_F( INFO, "Adding DetPlane at (0, 0, %0.2f)", z );
			DetPlanes.push_back( genfit::SharedPlanePtr(new genfit::DetPlane(TVector3(0, 0, z), TVector3(1, 0, 0), TVector3(0, 1, 0))) );
		}



		// get cfg values
		vertexSigmaXY = cfg.get<float>( "TrackFitter.Vertex:sigmaXY", 1 );
		vertexSigmaZ  = cfg.get<float>( "TrackFitter.Vertex:sigmaZ", 30 );
		vertexPos     = cfg.getFloatVector( "TrackFitter.Vertex:pos", 0, 3 );
		includeVertexInFit = cfg.get<bool>( "TrackFitter.Vertex:includeInFit", false );


		LOG_F( INFO, "vertex pos = (%f, %f, %f)", vertexPos[0], vertexPos[1], vertexPos[2] );
		LOG_F( INFO, "vertex sigma = (%f, %f, %f)", vertexSigmaXY, vertexSigmaXY, vertexSigmaZ );
		LOG_F( INFO, "Include vertex in fit %d", (int) includeVertexInFit );

		rand = new TRandom3();
		rand->SetSeed(0);

		// make a fixed cov mat for hits if we arent going to use the real errors
		double detectorResolution(cfg.get<float>( "TrackFitter.Hits:sigmaXY", 0.01 )); // resolution of sTGC detectors
		LOG_F( INFO, "Hit Covariance Matrix resolution = %f", detectorResolution  );

		FakeHitCov.ResizeTo(2, 2);
		FakeHitCov.UnitMatrix();
		FakeHitCov *= detectorResolution * detectorResolution;

		useFCM = cfg.get<bool>( "TrackFitter.Hits:useFCM", false );

		skipSi0 = cfg.get<bool>( "TrackFitter.Hits:skipSi0", false);
		skipSi1 = cfg.get<bool>( "TrackFitter.Hits:skipSi1", false);

		if ( skipSi0 ) {
			LOG_F( INFO, "Skip Si disk 0 : true" );
		}

		if ( skipSi1 ) {
			LOG_F( INFO, "Skip Si disk 1 : true" );
		}


		makeHistograms();

	}

	void makeHistograms()
	{
		std::string n = "";
		hist[ "ECalProjPosXY" ] 	= new TH2F( "ECalProjPosXY", ";X;Y", 1000, -500, 500, 1000, -500, 500 );
		hist[ "ECalProjSigmaXY" ] 	= new TH2F( "ECalProjSigmaXY", ";#sigma_{X};#sigma_{Y}", 50, 0, 0.5, 50, 0, 0.5 );

		hist[ "SiProjPosXY" ] 		= new TH2F( "SiProjPosXY", ";X;Y", 1000, -500, 500, 1000, -500, 500 );
		hist[ "SiProjSigmaXY" ] 	= new TH2F( "SiProjSigmaXY", ";#sigma_{X};#sigma_{Y}", 150, 0, 15, 150, 0, 15 );

		hist[ "SiWrongProjPosXY" ] 		= new TH2F( "SiWrongProjPosXY", ";X;Y", 1000, -500, 500, 1000, -500, 500 );
		hist[ "SiWrongProjSigmaXY" ] 	= new TH2F( "SiWrongProjSigmaXY", ";#sigma_{X};#sigma_{Y}", 50, 0, 0.5, 50, 0, 0.5 );

		hist[ "SiDeltaProjPosXY" ] 		= new TH2F( "SiDeltaProjPosXY", ";X;Y", 1000, 0, 20, 1000, 0, 20 );

		n = "seed_curv"; hist[ n ] 	= new TH1F( n.c_str(), ";curv", 1000, 0, 10000 );
		n = "seed_pT"; hist[ n ] 	= new TH1F( n.c_str(), ";pT (GeV/c)", 500, 0, 10 );
		n = "seed_eta"; hist[ n ] 	= new TH1F( n.c_str(), ";eta", 500, 0, 5 );

		n = "delta_fit_seed_pT"; hist[ n ] 		= new TH1F( n.c_str(), ";#Delta( fit, seed ) pT (GeV/c)", 500, -5, 5 );
		n = "delta_fit_seed_eta"; hist[ n ] 	= new TH1F( n.c_str(), ";#Delta( fit, seed ) eta", 500, 0, 5 );
		n = "delta_fit_seed_phi"; hist[ n ] 	= new TH1F( n.c_str(), ";#Delta( fit, seed ) phi", 500, -5, 5 );
	}

	void writeHistograms()
	{
		for (auto nh : hist ) {
			nh.second->Write();
		}
	}


	/* Get the Covariance Matrix for the detector hit point
	 *
	 */
	TMatrixDSym getCovMat( KiTrack::IHit *hit )
	{
		// we can calculate the CovMat since we know the det info, but in future we should probably keep this info in the hit itself

		// for sTGC it is simple to compute the cartesian cov mat, do that first.


		if ( hit->getSector() > 2 ) { // 0-2 are Si, 3-6 are sTGC
			// only need the 2x2 since these are "measurements on a plane"
			TMatrixDSym cm(2);
			const float sigmaXY = 100  * 1e-4; // 100 microns convert to cm
			cm(0, 0) = sigmaXY * sigmaXY;
			cm(1, 1) = sigmaXY * sigmaXY;
			return cm;
		}
		else {
			// measurements on a plane only need 2x2
			// for Si geom we need to convert from cyl to car coords
			TMatrixDSym cm(2);
			TMatrixD T(2, 2);
			const float x = hit->getX();
			const float y = hit->getY();
			const float R = sqrt( x * x + y * y );
			const float cosphi = x / R;
			const float sinphi = y / R;
			const float sqrt12 = sqrt(12.);
			const float r_size = 3.0; // cm
			const float dr = r_size / sqrt12;
			const float nPhiDivs = 128 * 12; // may change with geom?
			const float dphi = ( TMath::TwoPi() / (nPhiDivs)) / sqrt12;

			T(0, 0) = cosphi;
			T(0, 1) = -R * sinphi;
			T(1, 0) = sinphi;
			T(1, 1) = R * cosphi;

			TMatrixD cmcyl(2, 2);
			cmcyl(0, 0) = dr * dr;
			cmcyl(1, 1) = dphi * dphi;

			TMatrixD r = T * cmcyl * T.T();
			const float sigmaX = sqrt( r(0, 0) );
			const float sigmaY = sqrt( r(1, 1) );
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

	float fitSimpleCircle( vector<KiTrack::IHit *> trackCand, size_t i0, size_t i1, size_t i2 )
	{
		float curv = 0;

		if ( i0 > 12 || i1 > 12 || i2 > 12 )
			return 0;

		try {
			KiTrack::SimpleCircle sc( trackCand[i0]->getX(), trackCand[i0]->getY(), trackCand[i1]->getX(), trackCand[i1]->getY(), trackCand[i2]->getX(), trackCand[i2]->getY() );
			curv = sc.getRadius();
		}
		catch ( KiTrack::InvalidParameter  &e ) {
			LOG_F( ERROR, "Cannot get pT seed, only had %lu points", trackCand.size() );
		}

		if ( isinf(curv) )
			curv = 999999.9;

		LOG_F( INFO, "SimpleCircle Fit to (%lu, %lu, %lu) = %f", i0, i1, i2, curv );
		return curv;
	}

	float seedState( vector<KiTrack::IHit *> trackCand, TVector3 &seedPos, TVector3 &seedMom )
	{
		LOG_SCOPE_FUNCTION(INFO);

		// we require at least 4 hits,  so this should be gauranteed
		assert( trackCand.size() > 3 );

		TVector3 p0, p1, p2;

		// we want to use the LAST 3 hits, since silicon doesnt have R information
		// OK for now, since sTGC is 100% efficient so we can assume
		FwdHit *hit_closest_to_IP = static_cast<FwdHit *>(trackCand[0]);

		std::map<size_t, size_t> vol_map; // maps from <key=vol_id> to <value=index in trackCand>

		// init the map
		for ( size_t i = 0; i < 13; i++ ) vol_map[i] = -1;

		for ( size_t i = 0; i < trackCand.size(); i++ ) {
			auto fwdHit = static_cast<FwdHit *>(trackCand[i]);
			vol_map[ abs(fwdHit->_vid) ] = i;
			LOG_F( INFO, "HIT _vid=%d (%f, %f, %f)", fwdHit->_vid, fwdHit->getX(), fwdHit->getY(), fwdHit->getZ() );

			// find the hit closest to IP for the initial position seed
			if ( hit_closest_to_IP->getZ() > fwdHit->getZ() )
				hit_closest_to_IP = fwdHit;
		}

		// enumerate the partitions
		// 12 11 10
		// 12 11 9
		// 12 10 9
		// 11 10 9
		vector<float> curvs;
		curvs.push_back( fitSimpleCircle( trackCand, vol_map[12], vol_map[11], vol_map[10] ) );
		curvs.push_back( fitSimpleCircle( trackCand, vol_map[12], vol_map[11], vol_map[9 ] ) );
		curvs.push_back( fitSimpleCircle( trackCand, vol_map[12], vol_map[10], vol_map[9 ] ) );
		curvs.push_back( fitSimpleCircle( trackCand, vol_map[11], vol_map[10], vol_map[9 ] ) );

		// average them and exclude failed fits
		float mcurv = 0;
		float nmeas = 0;

		for ( size_t i = 0; i < curvs.size(); i++ ) {
			LOG_F( INFO, "curv[%lu] = %f", i, curvs[i] );

			if ( MAKE_HIST ) this->hist[ "seed_curv" ] -> Fill( curvs[i] );

			if ( curvs[i] > 10 ) {
				mcurv += curvs[i];
				nmeas += 1.0;
			}
		}

		if ( nmeas >= 1 )
			mcurv = mcurv / nmeas;
		else
			mcurv = 10;

		// if ( mcurv < 150 )
		// 	mcurv = 150;

		if ( vol_map[9 ] < 13 )
			p0.SetXYZ( trackCand[ vol_map[9 ] ]->getX(), trackCand[ vol_map[9 ] ]->getY(), trackCand[ vol_map[9 ] ]->getZ() );

		if ( vol_map[10] < 13 )
			p1.SetXYZ( trackCand[ vol_map[10] ]->getX(), trackCand[ vol_map[10] ]->getY(), trackCand[ vol_map[10] ]->getZ() );

		if ( p0.Mag() < 0.00001 || p1.Mag() < 0.00001 ) {
			LOG_F( ERROR, "bad input hits" );
		}

		const double K = 0.00029979; //K depends on the used units
		double pt = mcurv * K * 5;
		double dx = (p1.X() - p0.X());
		double dy = (p1.Y() - p0.Y());
		double dz = (p1.Z() - p0.Z());
		double phi = TMath::ATan2( dy, dx );
		double Rxy = sqrt( dx * dx + dy * dy );
		double theta =  TMath::ATan2(Rxy, dz);
		// double eta = -log( tantheta / 2.0 );
		//  probably not the best starting condition, but lets try it


		seedMom.SetPtThetaPhi( pt, theta, phi );
		seedPos.SetXYZ( hit_closest_to_IP->getX(), hit_closest_to_IP->getY(), hit_closest_to_IP->getZ() );

		if ( MAKE_HIST ) {
			this->hist[ "seed_pT" ] -> Fill( seedMom.Pt() );
			this->hist[ "seed_eta" ] -> Fill( seedMom.Eta() );
		}


		return mcurv;
	}

	genfit::MeasuredStateOnPlane projectTo( size_t si_plane, genfit::Track *fitTrack ){
		LOG_SCOPE_FUNCTION( INFO );
		if ( si_plane > 2 ){
			LOG_F( ERROR, "si_plane out of bounds = %lu", si_plane );
			genfit::MeasuredStateOnPlane nil;
			return nil;
		}

		// start with last Si plane closest to STGC
		LOG_F( INFO, "Projecting to Si Plane %lu", si_plane );
		auto detSi = SiDetPlanes[si_plane];
		genfit::MeasuredStateOnPlane tst = fitTrack->getFittedState(1);
		auto TCM = fitTrack->getCardinalRep()->get6DCov(tst);
		double len = fitTrack->getCardinalRep()->extrapolateToPlane( tst, detSi, false, true );

		LOG_F(INFO, "len to Si (Disk %lu) =%f", si_plane, len);
		LOG_F(INFO, "Position at Si (%0.2f, %0.2f, %0.2f) +/- (%0.2f, %0.2f, %0.2f)", tst.getPos().X(), tst.getPos().Y(), tst.getPos().Z(), sqrt(TCM(0, 0)), sqrt(TCM(1, 1)), sqrt(TCM(2, 2)) );
		TCM = fitTrack->getCardinalRep()->get6DCov( tst );
		return tst;
	}

	TVector3 refitTrackWithSiHits( genfit::Track *originalTrack, std::vector<KiTrack::IHit *> si_hits ){
		LOG_SCOPE_FUNCTION( INFO );

		
		

		TVector3  pOrig = originalTrack->getCardinalRep()->getMom(originalTrack->getFittedState(1, originalTrack->getCardinalRep()));
		auto cardinalStatus = originalTrack->getFitStatus(originalTrack->getCardinalRep());

		if ( originalTrack->getFitStatus( originalTrack->getCardinalRep() )->isFitConverged() == false ){
			LOG_F( WARNING, "Original Track fit did not converge, skipping" );
			return pOrig;
		}

		auto trackRepPos = new genfit::RKTrackRep(pdg_mu_plus);
		auto trackRepNeg = new genfit::RKTrackRep(pdg_mu_minus);

		auto trackPoints = originalTrack->getPointsWithMeasurement();

		TVectorD rawCoords = trackPoints[0]->getRawMeasurement()->getRawHitCoords();
		TVector3 seedPos( rawCoords(0), rawCoords(1), rawCoords(2));
		TVector3 seedMom = pOrig;
		auto refTrack = new genfit::Track(trackRepPos, seedPos, seedMom);
		refTrack->addTrackRep( trackRepNeg );

		genfit::Track &fitTrack = *refTrack;
		

		// initialize the hit coords on plane
		TVectorD hitCoords(2);
		hitCoords[0] = 0;
		hitCoords[1] = 0;

		int planeId(0);
		int hitId(0);

		// add the hits to the track
		for ( auto h : si_hits ){
			TMatrixDSym hitCovMat = FakeHitCov;

			if ( useFCM == false )
				hitCovMat = getCovMat( h );

			hitCoords[0] = h->getX();
			hitCoords[1] = h->getY();
			genfit::PlanarMeasurement *measurement = new genfit::PlanarMeasurement(hitCoords, hitCovMat, h->getSector(), ++hitId, nullptr);

			planeId = h->getSector();

			if ( SiDetPlanes.size() <= planeId ) {
				LOG_F(ERROR, "invalid VolumId -> out of bounds DetPlane, vid = %d", planeId );
				return pOrig;
			}

			auto plane = SiDetPlanes[ planeId ];
			measurement->setPlane( plane, planeId);
			fitTrack.insertPoint(new genfit::TrackPoint(measurement, &fitTrack));
		}

		LOG_F( INFO, "Track now has %lu points", fitTrack.getNumPoints() );

		LOG_F( INFO, "Is Track Fitted Already? %d", fitter->isTrackFitted( &fitTrack, fitTrack.getCardinalRep() ) );


		try {
			LOG_SCOPE_F(INFO, "Track RE-Fit with GENFIT2");
			fitTrack.checkConsistency();

			// do the fit
			fitter->processTrack(&fitTrack);

			{
				// print fit result
				LOG_SCOPE_F(INFO, "Fitted State AFTER Refit");
				fitTrack.getFittedState().Print();
			}
		}
		catch (genfit::Exception &e) {
			std::cerr << e.what();
			std::cerr << "Exception on track RE-fit" << std::endl;
		}


		
		if ( fitTrack.getFitStatus( fitTrack.getCardinalRep() )->isFitConverged() == false ){
			LOG_F( ERROR, "Fit with Si hits did not converge" );
		} else {
			TVector3  p = fitTrack.getCardinalRep()->getMom(fitTrack.getFittedState(1, fitTrack.getCardinalRep()));
			LOG_F( INFO, "FitMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", p.Pt(), p.Eta(), p.Phi() );
			LOG_F( INFO, "Original FitMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", pOrig.Pt(), pOrig.Eta(), pOrig.Phi() );
			auto newStatus = fitTrack.getFitStatus(fitTrack.getCardinalRep());
			LOG_F( INFO, "Chi2 before / after = %f / %f", cardinalStatus->getChi2(), newStatus->getChi2() );

			return p;
		}
		return pOrig;
	}


	void studyProjectionToSi( genfit::AbsTrackRep *cardinalRep, genfit::AbsTrackRep *wrongRep, genfit::Track &fitTrack )
	{

		// try projecting onto the Si plane
		const float DET_Z = 140;
		auto detSi = genfit::SharedPlanePtr( new genfit::DetPlane(TVector3(0, 0, DET_Z), TVector3(1, 0, 0), TVector3(0, 1, 0)) );
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

		double len = cardinalRep->extrapolateToPlane( tst, detSi, false, true );
		double len2 = wrongRep->extrapolateToPlane( tst2, detSi, false, true );

		LOG_F(INFO, "len to Si %f", len);
		LOG_F( INFO, "Position at Si (%0.2f, %0.2f, %0.2f) +/- ()", tst.getPos().X(), tst.getPos().Y(), tst.getPos().Z() );
		TCM = cardinalRep->get6DCov( tst );
		LOG_F( INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(0, 0), TCM(0, 1), TCM(0, 2), TCM(0, 3), TCM(0, 4), TCM(0, 5) );
		LOG_F( INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(1, 0), TCM(1, 1), TCM(1, 2), TCM(1, 3), TCM(1, 4), TCM(1, 5) );
		LOG_F( INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(2, 0), TCM(2, 1), TCM(2, 2), TCM(2, 3), TCM(2, 4), TCM(2, 5) );
		LOG_F( INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(3, 0), TCM(3, 1), TCM(3, 2), TCM(3, 3), TCM(3, 4), TCM(3, 5) );
		LOG_F( INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(4, 0), TCM(4, 1), TCM(4, 2), TCM(4, 3), TCM(4, 4), TCM(4, 5) );
		LOG_F( INFO, "Cov SI (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(5, 0), TCM(5, 1), TCM(5, 2), TCM(5, 3), TCM(5, 4), TCM(5, 5) );

		auto TCM2 = wrongRep->get6DCov( tst2 );

		if ( MAKE_HIST ) {

			this->hist[ "SiProjPosXY" ]->Fill( tst.getPos().X(), tst.getPos().Y() );
			this->hist[ "SiProjSigmaXY" ]->Fill( sqrt(TCM(0, 0)), sqrt(TCM(1, 1)) );

			this->hist[ "SiWrongProjPosXY" ]->Fill( tst2.getPos().X(), tst2.getPos().Y() );
			this->hist[ "SiWrongProjSigmaXY" ]->Fill( sqrt(TCM2(0, 0)), sqrt(TCM2(1, 1)) );

			LOG_F( INFO, "DeltaX Si Proj = %f", fabs( tst.getPos().X() - tst2.getPos().X()) );
			this->hist[ "SiDeltaProjPosXY" ]->Fill( fabs( tst.getPos().X() - tst2.getPos().X()), fabs( tst.getPos().Y() - tst2.getPos().Y()) );


		}

	}


	void studyProjectionToECal( genfit::AbsTrackRep *cardinalRep, genfit::Track &fitTrack )
	{

		// try projecting onto the Si plane
		auto detSi = genfit::SharedPlanePtr( new genfit::DetPlane(TVector3(0, 0, 711), TVector3(1, 0, 0), TVector3(0, 1, 0)) );
		genfit::MeasuredStateOnPlane tst = fitTrack.getFittedState(1);

		auto TCM = cardinalRep->get6DCov(tst);
		// LOG_F( INFO, "Position pre ex (%0.2f, %0.2f, %0.2f) +/- ()", cardinalRep->getPos(tst).X(), cardinalRep->getPos(tst).Y(), cardinalRep->getPos(tst).Z() );
		// LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(0,0), TCM(0,1), TCM(0,2), TCM(0,3), TCM(0,4), TCM(0,5) );
		// LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(1,0), TCM(1,1), TCM(1,2), TCM(1,3), TCM(1,4), TCM(1,5) );
		// LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(2,0), TCM(2,1), TCM(2,2), TCM(2,3), TCM(2,4), TCM(2,5) );
		// LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(3,0), TCM(3,1), TCM(3,2), TCM(3,3), TCM(3,4), TCM(3,5) );
		// LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(4,0), TCM(4,1), TCM(4,2), TCM(4,3), TCM(4,4), TCM(4,5) );
		// LOG_F( INFO, "Cov pre ex (%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f)", TCM(5,0), TCM(5,1), TCM(5,2), TCM(5,3), TCM(5,4), TCM(5,5) );

		double len = cardinalRep->extrapolateToPlane( tst, detSi, false, true );

		LOG_F(INFO, "len to ECal %f", len);
		LOG_F( INFO, "Position at ECal (%0.2f, %0.2f, %0.2f) +/- ()", tst.getPos().X(), tst.getPos().Y(), tst.getPos().Z() );
		TCM = cardinalRep->get6DCov( tst );
		LOG_F( INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(0, 0), TCM(0, 1), TCM(0, 2), TCM(0, 3), TCM(0, 4), TCM(0, 5) );
		LOG_F( INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(1, 0), TCM(1, 1), TCM(1, 2), TCM(1, 3), TCM(1, 4), TCM(1, 5) );
		LOG_F( INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(2, 0), TCM(2, 1), TCM(2, 2), TCM(2, 3), TCM(2, 4), TCM(2, 5) );
		LOG_F( INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(3, 0), TCM(3, 1), TCM(3, 2), TCM(3, 3), TCM(3, 4), TCM(3, 5) );
		LOG_F( INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(4, 0), TCM(4, 1), TCM(4, 2), TCM(4, 3), TCM(4, 4), TCM(4, 5) );
		LOG_F( INFO, "Cov ECAL (%0.5f, %0.5f, %0.5f, %0.5f, %0.5f, %0.5f)", TCM(5, 0), TCM(5, 1), TCM(5, 2), TCM(5, 3), TCM(5, 4), TCM(5, 5) );

		if ( MAKE_HIST ) {
			this->hist[ "ECalProjPosXY" ]->Fill( tst.getPos().X(), tst.getPos().Y() );
			this->hist[ "ECalProjSigmaXY" ]->Fill( sqrt(TCM(0, 0)), sqrt(TCM(1, 1)) );
		}


	}

	TVector3 fitTrack( vector<KiTrack::IHit *> trackCand, double *Vertex = 0 )
	{
		LOG_SCOPE_FUNCTION(INFO);

		LOG_F( INFO, "Track candidate size: %lu", trackCand.size() );

		// The PV information, if we want to use it
		TVectorD pv(3);

		if ( 0 == Vertex ) {
			pv[0] = vertexPos[0] + rand->Gaus( 0, vertexSigmaXY );
			pv[1] = vertexPos[1] + rand->Gaus( 0, vertexSigmaXY );
			pv[2] = vertexPos[2] + rand->Gaus( 0, vertexSigmaZ );
		}
		else {
			pv[0] = Vertex[0];
			pv[1] = Vertex[1];
			pv[2] = Vertex[2];
		}

		// get the seed info from our hits
		TVector3 seedMom, seedPos;
		float curv = seedState( trackCand, seedPos, seedMom );


		// create the track representations
		auto trackRepPos = new genfit::RKTrackRep(pdg_mu_plus);
		auto trackRepNeg = new genfit::RKTrackRep(pdg_mu_minus);

		// If we use the PV, use that as the start pos for the track
		if ( includeVertexInFit ) {
			seedPos.SetXYZ( pv[0], pv[1], pv[2] );
		}

		if ( fTrack ) delete fTrack;

		fTrack = new genfit::Track(trackRepPos, seedPos, seedMom);
		fTrack->addTrackRep( trackRepNeg );

		genfit::Track &fitTrack = *fTrack;

		const int detId(0); // detector ID
		int planeId(0); // detector plane ID
		int hitId(0); // hit ID



		/******************************************************************************************************************************
		* Include the Primary vertex if desired
		*******************************************************************************************************************************/
		if ( includeVertexInFit ) {

			TMatrixDSym hitCov3(3);
			hitCov3(0, 0) = vertexSigmaXY * vertexSigmaXY;
			hitCov3(1, 1) = vertexSigmaXY * vertexSigmaXY;
			hitCov3(2, 2) = vertexSigmaZ  * vertexSigmaZ;


			genfit::SpacepointMeasurement *measurement = new genfit::SpacepointMeasurement(pv, hitCov3, 0, ++hitId, nullptr);
			fitTrack.insertPoint(new genfit::TrackPoint(measurement, &fitTrack));
		}

		// initialize the hit coords on plane
		TVectorD hitCoords(2);
		hitCoords[0] = 0;
		hitCoords[1] = 0;

		/******************************************************************************************************************************
		 * loop over the hits, add them to the track
		 *******************************************************************************************************************************/
		for ( auto h : trackCand ) {

			// if ( skipSi0 == true && h->getSector() == 0 ) continue;
			// if ( skipSi1 == true && h->getSector() == 1 ) continue;

			// if ( static_cast<FwdHit*>(h)->_vid == 9 ) continue;
			// if ( static_cast<FwdHit*>(h)->_vid == 10 ) continue;
			// if ( static_cast<FwdHit*>(h)->_vid == 11 ) continue;


			TMatrixDSym hitCovMat = FakeHitCov;

			if ( useFCM == false )
				hitCovMat = getCovMat( h );

			hitCoords[0] = h->getX();
			hitCoords[1] = h->getY();
			genfit::PlanarMeasurement *measurement = new genfit::PlanarMeasurement(hitCoords, hitCovMat, h->getSector(), ++hitId, nullptr);

			planeId = h->getSector();

			if ( DetPlanes.size() <= planeId ) {
				LOG_F(ERROR, "invalid VolumId -> out of bounds DetPlane, vid = %d", planeId );
				return TVector3(0, 0, 0);
			}

			auto plane = DetPlanes[ planeId ];
			measurement->setPlane( plane, planeId);
			fitTrack.insertPoint(new genfit::TrackPoint(measurement, &fitTrack));
		}

		LOG_F( INFO, "TRACK has %u hits", fitTrack.getNumPointsWithMeasurement() );

		try {
			LOG_SCOPE_F(INFO, "Track Fit with GENFIT2");
			fitTrack.checkConsistency();

			// do the fit
			fitter->processTrack(&fitTrack);

			// print fit result
			fitTrack.getFittedState().Print();
		}
		catch (genfit::Exception &e) {
			std::cerr << e.what();
			std::cerr << "Exception on track fit" << std::endl;
		}

		LOG_F( INFO, "Get fit status and momentum" );
		TVector3 p(0, 0, 0);

		try {
			LOG_SCOPE_F(INFO, "Get Track Fit status and momentum etc.");
			//check
			fitTrack.checkConsistency();

			auto cardinalRep = fitTrack.getCardinalRep();
			auto cardinalStatus = fitTrack.getFitStatus(cardinalRep);
			fStatus   = *cardinalStatus; // save the status of last fit

			// Delete any previous track rep
			if ( fTrackRep ) delete fTrackRep;

			// Clone the cardinal rep for persistency
			fTrackRep = cardinalRep->clone(); // save the result of the fit

			if ( fitTrack.getFitStatus(trackRepPos)->isFitConverged() == false &&
					fitTrack.getFitStatus(trackRepNeg)->isFitConverged() == false ) {
				LOG_F( INFO, "*********************FIT SUMMARY*********************" );
				LOG_F( ERROR, "Track Fit failed to converge" );
				LOG_F( INFO, "SeedMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", seedMom.Pt(), seedMom.Eta(), seedMom.Phi() );
				LOG_F( INFO, "Estimated Curv: %f", curv );
				LOG_F( INFO, "SeedPosALT( X=%0.2f, Y=%0.2f, Z=%0.2f )", seedPos.X(), seedPos.Y(), seedPos.Z() );
				p.SetXYZ(0, 0, 0);
				return p;
			}

			// fStatus = *cardinalStatus; // save the status of last fit
			p = cardinalRep->getMom(fitTrack.getFittedState(1, cardinalRep));
			_q = cardinalRep->getCharge(fitTrack.getFittedState(1, cardinalRep));
			_p = p;


			/*
			  studyProjectionToECal( cardinalRep, fitTrack );

			if ( _q > 0 ){
				studyProjectionToSi(cardinalRep, trackRepNeg, fitTrack);
			} else if ( _q < 0 ) {
				studyProjectionToSi(cardinalRep, trackRepPos, fitTrack);
			}

			*/



		}
		catch (genfit::Exception &e) {
			LOG_F( INFO, "*********************FIT SUMMARY*********************" );
			std::cerr << e.what();
			std::cerr << "Exception on track fit" << std::endl;
			LOG_F( ERROR, "Track Fit failed" );
			LOG_F( INFO, "SeedMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", seedMom.Pt(), seedMom.Eta(), seedMom.Phi() );
			LOG_F( INFO, "Estimated Curv: %f", curv );
			LOG_F( INFO, "SeedPosALT( X=%0.2f, Y=%0.2f, Z=%0.2f )", seedPos.X(), seedPos.Y(), seedPos.Z() );
			p.SetXYZ(0, 0, 0);
			return p;
		}

		if ( makeDisplay ) {
			displayTracks.push_back( fitTrack );
			// event.push_back(&fitTrack);
			display->addEvent( &(displayTracks[displayTracks.size() - 1]) );
			// display->setOptions("ABDEFHMPT"); // G show geometry
		}

		LOG_F( INFO, "*********************FIT SUMMARY*********************" );
		LOG_F( INFO, "SeedMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", seedMom.Pt(), seedMom.Eta(), seedMom.Phi() );
		LOG_F( INFO, "Estimated Curv: %f", curv );
		LOG_F( INFO, "SeedPosALT( X=%0.2f, Y=%0.2f, Z=%0.2f )", seedPos.X(), seedPos.Y(), seedPos.Z() );
		LOG_F( INFO, "FitMom( pT=%0.2f, eta=%0.2f, phi=%0.2f )", p.Pt(), p.Eta(), p.Phi() );

		if ( MAKE_HIST ) {
			this->hist[ "delta_fit_seed_pT" ]->Fill( p.Pt() - seedMom.Pt() );
			this->hist[ "delta_fit_seed_eta" ]->Fill( p.Eta() - seedMom.Eta() );
			this->hist[ "delta_fit_seed_phi" ]->Fill( p.Phi() - seedMom.Phi() );
		}

		return p;
	}

	void showEvents()
	{
		if ( makeDisplay ) {
			display->setOptions("ABDEFGHMPT"); // G show geometry
			display->open();
		}
	}

	int getCharge()
	{
		return (int)_q;
	}

	genfit::FitStatus    getStatus()  { return fStatus;   }
	genfit::AbsTrackRep *getTrackRep() { return fTrackRep; }
	genfit::Track       *getTrack()   { return fTrack; }


private:
	jdb::XmlConfig &cfg;
	std::map<std::string, TH1 * > hist;
	bool MAKE_HIST = true;
	genfit::EventDisplay *display;
	std::vector<genfit::Track *> event;
	vector<genfit::Track> displayTracks;
	bool makeDisplay = false;
	genfit::AbsKalmanFitter *fitter = nullptr;

	const int pdg_pi_plus  = 211;
	const int pdg_pi_minus = -211;
	const int pdg_mu_plus  = 13;
	const int pdg_mu_minus = -13;

	genfit::AbsTrackRep *pion_track_rep = nullptr;
	vector<genfit::SharedPlanePtr> DetPlanes;
	vector<genfit::SharedPlanePtr> SiDetPlanes;
	bool useFCM = false;
	TMatrixDSym FakeHitCov;

	TRandom *rand = nullptr;

	// parameter ALIASED from cfg
	float vertexSigmaXY = 1;
	float vertexSigmaZ = 30;
	vector<float> vertexPos;
	bool includeVertexInFit = false;
	bool useSi = true;
	bool skipSi0 = false;
	bool skipSi1 = false;

	genfit::FitStatus    fStatus;
	genfit::AbsTrackRep *fTrackRep;
	genfit::Track       *fTrack;

	// Fit results
	TVector3 _p;
	double _q;

};



#endif
