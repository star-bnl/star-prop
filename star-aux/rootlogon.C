// Authors: Yuri Fisyak, Victor Perevozchikov, Jerome Lauret

{
  // #pragma optimize 0 <-- removed BuTracking 247
  //  set FloatPointException trap
  namespace rootlogon {
    int fpe=0;
    const char *env=0;
  }
  // eliminate the need to provide the custom ".rootrc" to activate Qt-layer
  if (gSystem->Getenv("WITH_QT") && gSystem->Getenv("QTDIR"))
  {
      // this allows to eliminate (or overwrite) the custom ".rootrc" file
      gEnv->SetValue("Gui.Backend","qt");
      gEnv->SetValue("Gui.Factory","qtgui");
  }

  rootlogon::fpe = TString(gSystem->Getenv("STAR_VERSION")) == ".DEV";
  rootlogon::env = gSystem->Getenv("STARFPE");

  if (rootlogon::env) {
    if (strcmp(rootlogon::env,"YES")==0) rootlogon::fpe=1;
    if (strcmp(rootlogon::env,"NO" )==0) rootlogon::fpe=0;
  }

  if (rootlogon::fpe) {
    gSystem->SetFPEMask(kInvalid | kDivByZero | kOverflow );
    printf("*** Float Point Exception is ON ***\n");
  } else {
    printf("*** Float Point Exception is OFF ***\n");
  }

  // Redefine prompt
  TString gPrompt =  gSystem->BaseName(gROOT->GetApplication()->Argv(0));
  gPrompt += " [%d] ";
  ((TRint*)gROOT->GetApplication())->SetPrompt( gPrompt.Data());

  // Load StarRoot lib.
  if (gSystem->DynamicPathName("libStarClassLibrary",kTRUE))
    gSystem->Load("libStarClassLibrary");

  if (!strstr(gSystem->GetLibraries(),"libTable")) {
    gSystem->Load("libGeom"); gSystem->Load("libTable");
  }

  gSystem->Load("libPhysics");
  gSystem->Load("libEG");
  gSystem->Load("libStarRoot");

  if (strstr(gSystem->GetLibraries(),"libTable")) {
    gInterpreter->ProcessLine("typedef TCL              StCL;");
    gInterpreter->ProcessLine("typedef TDataSet         St_DataSet ;");
    gInterpreter->ProcessLine("typedef TDataSetIter     St_DataSetIter;");
    gInterpreter->ProcessLine("typedef TFileSet         St_FileSet;");
    gInterpreter->ProcessLine("typedef TVolume          St_Node;");
    gInterpreter->ProcessLine("typedef TVolumePosition  St_NodePosition;");
    gInterpreter->ProcessLine("typedef TVolumeView      St_NodeView;");
    gInterpreter->ProcessLine("typedef TVolumeViewIter  St_NodeViewIter;");
    gInterpreter->ProcessLine("typedef TObjectSet       St_ObjectSet;");
    //  gInterpreter->ProcessLine("typedef TPointPosition   St_PointPosition;");
    gInterpreter->ProcessLine("typedef TPoints3D        St_Points3D;");
    gInterpreter->ProcessLine("typedef TPointsArray3D   St_PointsArray3D;");
    gInterpreter->ProcessLine("typedef TPolyLineShape   St_PolyLineShape;");
    gInterpreter->ProcessLine("typedef TTable           St_Table;");
    gInterpreter->ProcessLine("typedef TTable3Points    St_Table3Points;");
    gInterpreter->ProcessLine("typedef TTableIter       St_TableIter;");
    gInterpreter->ProcessLine("typedef TTablePoints     St_TablePoints;");
    gInterpreter->ProcessLine("typedef TTableSorter     St_TableSorter;");
    gInterpreter->ProcessLine("typedef TTableDescriptor St_tableDescriptor;");
  }

  printf(" *** Start at Date : %s\n",TDatime().AsString());

  if (gROOT->IsBatch()==0 && gSystem->Getenv("OPENGL")) gROOT->Macro("GL.C");

  gROOT->SetStyle("Plain");// Default white background for all plots

  // The modes below are provided by Nick van Eijndhoven <Nick@phys.uu.nl>
  // from Alice.
  gStyle->SetCanvasColor(10);
  gStyle->SetStatColor(10);
  gStyle->SetTitleFillColor(10);
  gStyle->SetPadColor(10);

  // Settings for statistics information
  gStyle->SetOptFit(1);
  gStyle->SetOptStat(1);

  // SetPaperSize wants width & height in cm: A4 is 20,26 & US is 20,24
  gStyle->SetPaperSize(20,24);

  // Positioning of axes labels
  gStyle->SetTitleOffset(1.2);
  // grid
  gStyle->SetPadGridX(1);
  gStyle->SetPadGridY(1);

  //  Set date/time for plot
  gStyle->SetOptDate(1);

  // Assign large size of hashtable for STAR I/O
  //TBuffer::SetGlobalWriteParam(2003);
  TBufferFile::SetGlobalWriteParam(2003); // value is a large prime

  // ROOT and XROOTD
  // some rootd default dummy stuff
  TAuthenticate::SetGlobalUser("starlib");
  TAuthenticate::SetGlobalPasswd("ROOT4STAR");

  // This is already implied in system.rootrc although one could use
  // this to switch to a beta version of the client library.
  //ROOT->GetPluginManager()->AddHandler("TFile","^root:","TXNetFile", "Netx", "TXNetFile(const char*,Option_t*,const char*,Int_t,Int_t)");

  // This will help tracing failure on XrdOpen() if any
  gEnv->SetValue("XNet.DebugTimestamp","1");
  //gEnv->SetValue("XNet.ConnectDomainAllowRE","rcf.bnl.gov|usatlas.bnl.gov");
  //gEnv->SetValue("XNet.RedirDomainAllowRE","rcf.bnl.gov");
  gEnv->SetValue("XNet.ReconnectTimeout","5");

  //  Print version
  namespace _rootlogon_ {
    TString STAR_LEVEL("$STAR_LEVEL");
    TString ROOT_LEVEL("$ROOT_LEVEL");
    int dumy = gSystem->ExpandPathName(STAR_LEVEL);
    int dumy = gSystem->ExpandPathName(ROOT_LEVEL);
    int dumy = printf("QAInfo:You are using STAR_LEVEL : %s, ROOT_LEVEL : %s and node : %s \n",
              STAR_LEVEL.Data(),ROOT_LEVEL.Data(),gSystem->HostName());
  }

  // note that the above bacward support the old mode for include whenever
  // it was not in .$STAR_HOST_SYS but one level up. The backward compatibility
  // can be removed only at the net root release ...
  gSystem->SetIncludePath(" -I.");
  gSystem->AddIncludePath(" -I./.$STAR_HOST_SYS/include -I./StRoot -I$STAR/.$STAR_HOST_SYS/include -I$STAR/StRoot -I/usr/include/mysql");
  if (gSystem->Getenv("QTDIR"))
     gSystem->AddIncludePath(" -I$QTDIR/include -I$QTDIR/include/Qt -I$QTDIR/include/QtCore -I$QTDIR/include/QtGui");
  gSystem->AddIncludePath(" -I$ROOTSYS/include");

  // massage of compilation option - note that std=c++0x has the meaning
  // of std=c++11 for compiler 4.8 and above so this is "safe"
  TString SavedCompilationCommand(gSystem->GetMakeSharedLib());
  TString NewCompilationCommand = SavedCompilationCommand.ReplaceAll("std=c++11","std=c++0x");
  //cout << "Setting " << NewCompilationCommand.Data() << endl;
  gSystem->SetMakeSharedLib(NewCompilationCommand);


  // Requested by users, allow loading a "complementary" local
  // rootlogon if exists
  if ( !gSystem->AccessPathName("./rootlogon.C", kFileExists ) ){
    cout << "Found and loading local rootlogon.C" << endl;
    gROOT->Macro("./rootlogon.C");
  }

}
