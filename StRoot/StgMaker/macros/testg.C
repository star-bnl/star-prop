TString infile = "testg.fzd";

void testg( size_t n_events = 1000, const char *filename = 0, const char* config="config.xml" )
{
   if (filename) infile = filename;

   gROOT->LoadMacro("bfc.C");
// bfc(0, "fzin agml debug vfmce mcevent makeevent stu sdt20181215 cmudst", infile );
// bfc(0, "fzin agml       vfmce mcevent makeevent stu sdt20181215 cmudst", infile );
   bfc(0, "fzin agml       vfmce         makeevent stu sdt20181215 cmudst", infile );

   gSystem->Load("libgenfit2.so");
   gSystem->Load("libKiTrack.so");
   gSystem->Load("libStgMaker.so");

   if ( filename ) cout << filename << endl;

   // Force build of the geometry
   TFile *geom = TFile::Open("fGeom.root");

   if (!geom) {
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

   TString logname = filename;
   logname.ReplaceAll("fzd","stg.log");

   if ( 1 ) {
     
     // Create genfit forward track maker and add it to the chain before the MuDst maker
     StgMaker *gmk = new StgMaker();
     gmk->SetAttr("useSTGC",1);
     gmk->SetAttr("useFSI",1);
     gmk->SetAttr("logfile",logname.Data());
     gmk->SetAttr("fillEvent",1);
     gmk->SetAttr("config",config); 
     //gmk->SetAttr("useFSI",1); 
     chain->AddAfter( "0Event", gmk );
     
     // And initialize it, since we have already initialized the chain
     gmk->Init();
     
   }
     
   // Do an ls to be sure
   chain->ls(3);

   size_t count = 0;

   // Loop over all events in the file...
   int stat = 0;

   while (stat == 0 && count < n_events) {

      cout << "===============================================================================" << endl;
      cout << "===============================================================================" << endl;
      cout << "Processing event number " << count++ << endl;
      cout << "===============================================================================" << endl;
      cout << "===============================================================================" << endl;

      chain->Clear();
      stat =    chain->Make();

      if (stat) break;

   }
}
