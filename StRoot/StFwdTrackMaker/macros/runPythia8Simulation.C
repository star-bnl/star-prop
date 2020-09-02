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

class StarPythia8;
StarPythia8* pythia8 = 0;

class StarParticleFilter;
StarParticleFilter* filter = 0;

int nevent = 0;

const char* libpythia8 = "libPythia8_1_62.so";


//______________________________________________________________________________
void geometry( TString tag, Bool_t agml=true ){
  TString cmd = "DETP GEOM "; cmd += tag;
  if ( !geant_maker ) geant_maker = (St_geant_Maker *)chain->GetMaker("geant");
  geant_maker -> LoadGeometry(cmd);
}
//______________________________________________________________________________
void command( TString cmd ){
  if ( !geant_maker ) geant_maker = (St_geant_Maker *)chain->GetMaker("geant");
  geant_maker -> Do( cmd );
}
//______________________________________________________________________________
void Pythia8( TString mode="DY:gamma*:ee" ){

  gSystem->Load( libpythia8 );

  pythia8 = new StarPythia8();
  pythia8->SetFrame("CMS", 510.0);
  pythia8->SetBlue("proton");
  pythia8->SetYell("proton");

  if ( mode == "DY:gamma*:ee" ) {

    pythia8->Set("SoftQCD:minBias = off");
    pythia8->Set("WeakSingleBoson:ffbar2gmZ=on");
    pythia8->Set("WeakZ0:gmZmode=1"); // 0=full / 1=gamma* / 2=Z0
    // Note: irrespective of the option used, the particle produced will always be assigned code 23 for Z^0, 
    // and open decay channels is purely dictated by what is set for the Z^0
    pythia8->Set("23:onMode=0");              // switch off all W+/- decaus
    pythia8->Set("23:onIfAny 11 -11");        // switch on for decays to e+/-
    pythia8->Set("PhaseSpace:mHatMin=4.0"); // M>4 GeV

    // Requires an electron OR positron in the forward tracker acceptance
    filter = new StarParticleFilter();
    filter->AddTrigger(  11, 0, -1, 2.5, 4.0 );
    filter->AddTrigger( -11, 0, -1, 2.5, 4.0 );

  }
    
  _primary -> AddGenerator(pythia8);  
  _primary -> SetCuts( 1.0E-6 , -1., -1.75, +5.250 );

  if ( filter ) _primary->AddFilter( filter );

}
//______________________________________________________________________________
void trig( const int n=1 ) {
  //  for ( Int_t i=0; i<n; i++ ) {
  TGiant3 *g3 = TGiant3::Geant3();
  int i=0, count=0;
  while( count < n )
  {
    // Clear the chain from the previous event
    chain->Clear();
    chain->Make();
    //    _primary->event()->Print(); 
    count++;
  }
}
//______________________________________________________________________________
TString _geometry = "";
TString _chain    = " gstar usexgeom xgeometry -geometry agml db detdb";

void runPythia8Simulation
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
  TString _outfile = Form("pythia8_%s_%ievts.fzd",gSystem->Getenv("JOBID"),nevents);


  gROOT->ProcessLine(".L bfc.C");
  bfc(0, _chain );

  std::cout << "Chain    : " << _chain.Data() << std::endl;

  gSystem->Load( "libVMC.so");
  gSystem->Load( "StarGeneratorUtil.so" );
  gSystem->Load( "StarGeneratorEvent.so" );
  gSystem->Load( "StarGeneratorBase.so" );
  gSystem->Load( "StarGeneratorFilt.so" );
 
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

  Pythia8();


 
  //
  // Initialize primary event generator and all sub makers
  //
  _primary -> Init();
 
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
//______________________________________________________________________________

