# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

o2_add_library(EventVisualisationBase
               SOURCES src/ConfigurationManager.cxx
                       src/DataInterpreter.cxx
                       src/DataSourceOffline.cxx
                       src/EventRegistration.cxx
                       src/GeometryManager.cxx
                       src/Track.cxx
               PUBLIC_LINK_LIBRARIES ROOT::Eve
                                     O2::CCDB 
                                     O2::EventVisualisationDataConverter
									 O2::DetectorsBase 

)
