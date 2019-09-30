// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// @file   TracksTest.h
/// @author Stefan Heckel, sheckel@cern.ch
///

#ifndef AliceO2_TPC_TRACKSTEST_H
#define AliceO2_TPC_TRACKSTEST_H

//root includes
#include "TH1F.h"
//o2 includes
#include "DataFormatsTPC/Defs.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "TPCBase/Sector.h"

namespace o2
{
namespace tpc
{
namespace qc
{

/// Keep QC information for track related observables - just a test
///
/// This is just a dummy implementation to get started with QC
/// @author Stefan Heckel, sheckel@cern.ch
class TracksTest
{
 public:
  TracksTest();

  bool processTrack(o2::tpc::TrackTPC const& track);

  /// Initialize all histograms
  void initializeHistograms();

  /// Reset all histograms
  void resetHistograms();

  std::vector<TH1F>& getHistograms1D() { return mHist1D; }
  const std::vector<TH1F>& getHistograms1D() const { return mHist1D; }

 private:
  std::vector<TH1F> mHist1D;

  ClassDefNV(TracksTest, 1)
};
} // namespace qc
} // namespace tpc
} // namespace o2

#endif
