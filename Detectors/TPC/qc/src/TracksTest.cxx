// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include <cmath>

// #include "DataFormatsTPC/dEdxInfo.h"

#include "TPCQC/TracksTest.h"

ClassImp(o2::tpc::qc::TracksTest);

using namespace o2::tpc::qc;

TracksTest::TracksTest() : mHist1D{}
{
}

void TracksTest::initializeHistograms()
{
  mHist1D.emplace_back("hNCls", "Number of clusters;# TPC clusters", 160, -0.5, 159.5);
}

void TracksTest::resetHistograms()
{
  for (auto& hist : mHist1D) {
    hist.Reset();
  }
}

bool TracksTest::processTrack(o2::tpc::TrackTPC const& track)
{
  // ===| variables required for cutting and filling |===
  const auto pT = track.getPt();
  const auto nCls = track.getNClusterReferences();

  // ===| cuts |===
  // hard coded cuts. Should be more configural in future
  if (pT < 0.15) {
    return false;
  }

  // ===| histogram filling |===
  mHist1D[0].Fill(nCls);

  return true;
}
