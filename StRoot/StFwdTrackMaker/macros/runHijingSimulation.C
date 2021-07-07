// macro to instantiate the Geant3 from within
// STAR  C++  framework and get the starsim prompt
// To use it do
//  root4star starsim.C

// WIDE z-vertx
#define __WIDE__ 

class St_geant_Maker;
St_geant_Maker *geant_maker = 0;

class StarGenEvent;
StarGenEvent   *event       = 0;

class StarPrimaryMaker;
StarPrimaryMaker *_primary = 0;

class StarHijing;
StarHijing* hijing = 0;

const bool dostepper = false;

TF1 *ptDist  = 0;
TF1 *etaDist = 0;
int nevent = 0;

// ----------------------------------------------------------------------------
void geometry( TString tag, Bool_t agml=true ){
  TString cmd = "DETP GEOM "; cmd += tag;
  if ( !geant_maker ) geant_maker = (St_geant_Maker *)chain->GetMaker("geant");
  geant_maker -> LoadGeometry(cmd);
  //  if ( agml ) command("gexec $STAR_LIB/libxgeometry.so");
}
// ----------------------------------------------------------------------------
void command( TString cmd ){
  if ( !geant_maker ) geant_maker = (St_geant_Maker *)chain->GetMaker("geant");
  geant_maker -> Do( cmd );
}
// ----------------------------------------------------------------------------
void Hijing(){

  gSystem->Load( "libHijing1_383.so");

  hijing = new StarHijing("hijing");
  hijing->SetTitle("Hijing 1.383");

  // Setup collision frame, energy and beam species
  hijing->SetFrame("CMS",200.0);
  hijing->SetBlue("Au");
  hijing->SetYell("Au");  
  hijing->SetImpact(0.0, 30.0);       // Impact parameter min/max (fm)    0.   30.

  _primary -> AddGenerator(hijing);
  _primary -> SetCuts( 1.0E-6 , -1., -1.75, +4.50 );

}
// ----------------------------------------------------------------------------
void trig( const int n=1 ) {
  //  for ( Int_t i=0; i<n; i++ ) {
  TGiant3 *g3 = TGiant3::Geant3();
  int i=0, count=0;
  while( count < n )
  {
    // Clear the chain from the previous event
    chain->Clear();
    chain->Make();
    count++;
  }
}
// ----------------------------------------------------------------------------
TString _geometry = "";
TString _chain    = " gstar usexgeom xgeometry -geometry agml db detdb";

void runHijingSimulation
( const int   nevents   = 1,
  const int   run       = 1,
  const char* alignment = "ideal",
  const char* timestamp = "sdt20150216",
  const char* mygeom    = "dev2021" 
  )
{ 
  
  _geometry = mygeom;
  _chain    = _geometry + _chain; 

  // Set random number generator seed
  int rngSeed = run + 1;

  // Set name of output file
  //  const char* _field = (0==field)?"BzConst":"BzStd";
  TString _outfile = Form("hijing_%s_%ievts.fzd",gSystem->Getenv("JOBID"),nevents);


  gROOT->ProcessLine(".L bfc.C");
  bfc(0, _chain );

  std::cout << "Chain    : " << _chain.Data() << std::endl;

  gSystem->Load( "libVMC.so");
  gSystem->Load( "StarGeneratorUtil.so" );
  gSystem->Load( "StarGeneratorEvent.so" );
  gSystem->Load( "StarGeneratorBase.so" );

  gSystem->Load( "libMathMore.so"   );  

  // Setup RNG seed and map all ROOT TRandom here
  StarRandom::seed( rngSeed );
  StarRandom::capture();
  
#if 1

  //
  // Create the primary event generator and insert it
  // before the geant maker
  //
  //  StarPrimaryMaker *
  _primary = new StarPrimaryMaker();  {
    _primary -> SetFileName( "kinematics.starsim.root");
    chain -> AddBefore( "geant", _primary );
  }

  //  Kinematics();
  Hijing();

  //
  // Initialize primary event generator and all sub makers
  //
  _primary -> Init();

  //
  // Need to set hijing parametes _after_ initialization
  //
  HiParnt_t &hiparnt = hijing->hiparnt();
  hiparnt.ihpr2(4)  = 0;    // Jet quenching (1=yes/0=no)       0
  hiparnt.ihpr2(3)  = 0;    // Hard scattering (1=yes/0=no)
  hiparnt.hipr1(10) = 2.0;  //    pT jet
  hiparnt.ihpr2(8)  = 10;   // Max number of jets / nucleon
  hiparnt.ihpr2(11) = 1;    // Set baryon production
  hiparnt.ihpr2(12) = 1;    // Turn on/off decay of particles [1=off and recommended]
  hiparnt.ihpr2(18) = 0;    // 1=B quark production.  0=C quark production.
  hiparnt.hipr1(7) = 5.35;  // Set B production ???? Not really used... Really ????

  hiparnt.ihpr2(21) = 0; // no need to keep decay chain (geant responsible, bad default)
  /*
  cout << "/HIPARNT/ ihpr2(11) = " << hiparnt.ihpr2(11) << endl;
  cout << "/HIPARNT/ ihpr2(12) = " << hiparnt.ihpr2(12) << endl;
  cout << "/HIPARNT/ ihpr2(18) = " << hiparnt.ihpr2(18) << endl;
  cout << "/HIPARNT/ ihpr2(21) = " << hiparnt.ihpr2(21) << endl;
  */
 
  //
  // Set primary vertex width to zero.  Cosmic ray gener will create vertex.
  //

  // Setup a narrow 5cm vertex distribution
  _primary->SetVertex(0,0,0);
  _primary->SetSigma(0.,0.,5.); 
  _primary->SetZvertexRange( -5., 5. ); // HFT trigger
  _primary->SetEtaRange(-1.75,5.250);


#endif

  //
  // Setup geometry and set starsim to use agusread for input
  //
  geometry( _geometry );


  //  command("aguser/gkine  -2    0    0.    650.0");
  //  command("user/input please evgen.1.nt");
  command("aguser/gkine -4 0");

  command(Form("gfile o %s",_outfile.Data()));
  
  //
  // Trigger on nevents
  //
  trig( nevents );

  command("call agexit");  // Make sure that STARSIM exits properly

}
// ----------------------------------------------------------------------------

