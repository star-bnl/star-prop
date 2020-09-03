#ifndef genfit_STARField_h
#define genfit_STARField_h

#include "TVector3.h"
#include "TFile.h"
#include "TH3.h"

#include "StarMagField/StarMagField.h"

#include "StFwdTrackMaker/include/Tracker/loguru.h"
#include "GenFit/AbsBField.h"

// hack of a global field pointer
genfit::AbsBField *_gField = 0;

//_______________________________________________________________________________________
// Adaptor for STAR magnetic field loaded via StarMagField Maker
class StarFieldAdaptor : public genfit::AbsBField {
  public:
    StarFieldAdaptor() {
        _gField = this; // global pointer in TrackFitter.h
    };
    virtual TVector3 get(const TVector3 &position) const {
        double x[] = {position[0], position[1], position[2]};
        double B[] = {0, 0, 0};

        if (StarMagField::Instance())
            StarMagField::Instance()->Field(x, B);

        return TVector3(B);
    };
    virtual void get(const double &_x, const double &_y, const double &_z, double &Bx, double &By, double &Bz) const {
        double x[] = {_x, _y, _z};
        double B[] = {0, 0, 0};

        if (StarMagField::Instance())
            StarMagField::Instance()->Field(x, B);

        Bx = B[0];
        By = B[1];
        Bz = B[2];
        return;
    };
};

namespace genfit
{

//_______________________________________________________________________________________
// Adaptor for STAR magnetic field loaded from a ROOT file with the field in cartesian coords
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
