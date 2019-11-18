#ifndef genfit_STARField_h
#define genfit_STARField_h

#include "TVector3.h"
#include "TFile.h"
#include "TH3.h"

#include "StgMaker/include/Tracker/loguru.h"
#include "GenFit/AbsBField.h"


namespace genfit
{

class STARField : public genfit::AbsBField
{
public:

   STARField()
   {
      LOG_F( INFO, "Loading STAR Magnetic Field map" );
      fField = new TFile( "FieldOn.root" );

      hFieldR   = (TH3 *)fField->Get( "fieldR" );
      hFieldY = (TH3 *)fField->Get( "fieldP" );
      hFieldZ   = (TH3 *)fField->Get( "fieldZ" );
   }
   virtual ~STARField() {;}

   virtual void fromCartesian( TVector3 XYZ, TVector3 &CYL ) const
   {
      CYL.SetX( sqrt( pow(XYZ.X(), 2) + pow(XYZ.Y(), 2) ) );
      CYL.SetY( TMath::ATan2( XYZ.Y(), XYZ.X() ) );
      // convert to degrees 0 -> 360
      CYL.SetY( CYL.Y() / ( 3.1415926 ) * 180. );

      if ( CYL.Y() < 0 ) CYL.SetY( CYL.Y() + 360 );

      CYL.SetZ( XYZ.Z() );
      return;
   }
   // virtual void toCartesian(TVector3 CYL, TVector3 &XYZ){
   // 	XYZ.SetX( CYL.X() * cos( CYL.Y() ) );
   // 	XYZ.SetY( CYL.X() * sin( CYL.Y() ) );
   // 	XYZ.SetZ( CYL.Z() );
   // 	return;
   // }

   virtual void get(const double &x, const double &y, const double &z, double &Bx, double &By, double &Bz) const
   {
      TVector3 pos( x, y, z );
      TVector3 field_ = get( pos );
      Bx = field_.X();
      By = field_.Y();
      Bz = field_.Z();
   }

   virtual TVector3 get(const TVector3 &position) const
   {
      assert( hFieldR != nullptr );
      assert( hFieldY != nullptr );
      assert( hFieldZ != nullptr );

      // we just need to convert to/from cyl coords
      // note the tree histograms have the same binning, so we only need to lookup once
      TVector3 CYL(0, 0, 0);
      fromCartesian( position, CYL );
      int bin_field = hFieldR->FindBin( CYL.X(), CYL.Y(), CYL.Z());
      TVector3 Brpz;
      Brpz.SetXYZ( hFieldR->GetBinContent( bin_field ), hFieldY->GetBinContent( bin_field ), hFieldZ->GetBinContent( bin_field ) );
      TVector3 Bxyz( 	cos(CYL.Y()) * Brpz.X() - sin(CYL.Y()) * Brpz.Y(),
                        sin(CYL.Y()) * Brpz.X() + cos(CYL.Y()) * Brpz.Y(),
                        Brpz.Z() );

      // LOG_F( INFO, "@(%f, %f, %f) ==> B( %f, %f, %f )", position.X(), position.Y(), position.Z() , Bxyz.X(), Bxyz.Y(), Bxyz.Z());
      return Bxyz;
   }

   TFile *fField = nullptr;
   TH3 *hFieldZ, *hFieldR, *hFieldY;
};


class STARFieldXYZ : public genfit::AbsBField
{
public:

   STARFieldXYZ()
   {
      LOG_F( INFO, "Loading STAR Magnetic Field map" );
      fField = new TFile( "FieldOnXYZ.root" );

      hFieldX   = (TH3 *)fField->Get( "fieldX" );
      hFieldY = (TH3 *)fField->Get( "fieldY" );
      hFieldZ   = (TH3 *)fField->Get( "fieldZ" );
   }
   virtual ~STARFieldXYZ() {;}

   virtual void fromCartesian( TVector3 XYZ, TVector3 &CYL ) const
   {
      CYL.SetX( sqrt( pow(XYZ.X(), 2) + pow(XYZ.Y(), 2) ) );
      CYL.SetY( TMath::ATan2( XYZ.Y(), XYZ.X() ) );
      // convert to degrees 0 -> 360
      CYL.SetY( CYL.Y() / ( 3.1415926 ) * 180. );

      if ( CYL.Y() < 0 ) CYL.SetY( CYL.Y() + 360 );

      CYL.SetZ( XYZ.Z() );
      return;
   }
   // virtual void toCartesian(TVector3 CYL, TVector3 &XYZ){
   // 	XYZ.SetX( CYL.X() * cos( CYL.Y() ) );
   // 	XYZ.SetY( CYL.X() * sin( CYL.Y() ) );
   // 	XYZ.SetZ( CYL.Z() );
   // 	return;
   // }

   virtual void get(const double &x, const double &y, const double &z, double &Bx, double &By, double &Bz) const
   {
      TVector3 pos( x, y, z );
      TVector3 field_ = get( pos );
      Bx = field_.X();
      By = field_.Y();
      Bz = field_.Z();
   }

   virtual TVector3 get(const TVector3 &position) const
   {
      assert( hFieldX != nullptr );
      assert( hFieldY != nullptr );
      assert( hFieldZ != nullptr );

      int bin_field = hFieldX->FindBin( position.X(), position.Y(), position.Z());
      TVector3 B;
      B.SetXYZ( hFieldX->GetBinContent( bin_field ), hFieldY->GetBinContent( bin_field ), hFieldZ->GetBinContent( bin_field ) );
      return B;
   }

   TFile *fField = nullptr;
   TH3 *hFieldZ, *hFieldX, *hFieldY;
};

}


#endif
