TString infile = "testg.fzd";
void testg( size_t n_events = 1000, const char *filename = 0 )
{

   if (filename) infile = filename;

   gROOT->LoadMacro("bfc.C");
   bfc(0, "fzin agml debug sti makeevent stu sdt20181215 cmudst", infile );
   gSystem->Load("libgenfit2.so");
   gSystem->Load("libKiTrack.so");
   gSystem->Load("libStgMaker.so");

   if ( filename ) cout << filename << endl;

   // Force build of the geometry
   TFile *geom = TFile::Open("fGeom.root");

   if ( 0 == geom ) {
      AgModule::SetStacker( new StarTGeoStacker() );
      AgPosition::SetDebug(2);
      StarGeometry::Construct("dev2021");

      // Believe that genfit requires the geometry is cached in a ROOT file
      gGeoManager->Export("fGeom.root");
   }
   else {
      cout << "WARNING:  Using CACHED geometry as a convienence!" << endl;
      delete geom;
   }

   // Create genfit forward track maker and add it to the chain before the MuDst maker
   StgMaker *gmk = new StgMaker();
   chain->AddAfter( "0Event", gmk );

   // And initialize it, since we have already initialized the chain
   gmk->Init();

   // Do an ls to be sure
   chain->ls(3);

   size_t count = 0;

   // Loop over all events in the file...
   int stat = 0;

   while (stat == 0 && count < n_events) {

      cout << "===============================================================================" << endl;
      cout << "===============================================================================" << endl;
      cout << endl << endl;
      cout << "Processing event number " << count++ << endl << endl;
      cout << "===============================================================================" << endl;
      cout << "===============================================================================" << endl;

      chain->Clear();
      stat =    chain->Make();

      if (stat) break;

   }

}
