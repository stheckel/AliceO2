#if !defined(__CLING__) || defined(__ROOTCLING__)
#include <iostream>
#include "TSystem.h"
#endif

void rootlogon()
{
  cout << endl;
  cout << endl;
  cout << "ATTENTION: ./rootlogon.C has been used !\n";
  cout << endl;
  cout << endl;

  gSystem->SetIncludePath("-I$O2_ROOT/../../ms_gsl/latest/include -I$O2_ROOT/../../FairLogger/latest/include -I$O2_ROOT/../../FairMQ/latest/include/fairmq");
}
