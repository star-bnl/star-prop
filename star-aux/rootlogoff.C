// Authors: Yuri Fisyak, Victor Perevozchikov, Jerome Lauret

{
  class StMaker;
  namespace rootlogoff {
    ::StMaker *mk=0;
  }

  if (TClassTable::GetDict("StMaker")) 
  {
    rootlogoff::mk = StMaker::GetChain();
    if (rootlogoff::mk) {
      rootlogoff::mk->Finish();
      if (TString(gSystem->Getenv("STAR_VERSION")) == ".DEV"
        && !gSystem->Getenv("STARNODELETE")) delete rootlogoff::mk;
      else  printf ("*** Chain not deleted***\n");
    }
  }
  printf("\nThis is the end of ROOT -- Goodbye\n\n");
}
