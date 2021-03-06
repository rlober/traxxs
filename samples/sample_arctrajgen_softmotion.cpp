// This file is a part of the traxxs framework.
// Copyright 2018, AKEOLAB S.A.S.
// Main contributor(s): Aurelien Ibanez, aurelien@akeo-lab.com
// 
// This software is a computer program whose purpose is to help create and manage trajectories.
// 
// This software is governed by the CeCILL-C license under French law and
// abiding by the rules of distribution of free software.  You can  use, 
// modify and/ or redistribute the software under the terms of the CeCILL-C
// license as circulated by CEA, CNRS and INRIA at the following URL
// "http://www.cecill.info". 
// 
// As a counterpart to the access to the source code and  rights to copy,
// modify and redistribute granted by the license, users are provided only
// with a limited warranty  and the software's author,  the holder of the
// economic rights,  and the successive licensors  have only  limited
// liability. 
// 
// In this respect, the user's attention is drawn to the risks associated
// with loading,  using,  modifying and/or developing or reproducing the
// software by the user in light of its specific status of free software,
// that may mean  that it is complicated to manipulate,  and  that  also
// therefore means  that it is reserved for developers  and  experienced
// professionals having in-depth computer knowledge. Users are therefore
// encouraged to load and test the software's suitability as regards their
// requirements in conditions enabling the security of their systems and/or 
// data to be ensured and,  more generally, to use and operate it in the 
// same conditions as regards security. 
// 
// The fact that you are presently reading this means that you have had
// knowledge of the CeCILL-C license and that you accept its terms.

#include <iostream>
#include <traxxs/impl/traxxs_softmotion/traxxs_softmotion.hpp>

/**
 * Demonstrates the softMotion implementation of traxxs::arc::ArcTrajGen
 */
int main(void) {
  ArcTrajGenSoftMotion arcTrajGen;
  traxxs::arc::ArcConditions c_i, c_f, c_max;
  traxxs::arc::ArcConditions c_cur;
  
  double dt = 0.05;
  
  // create an arc with initial/final conditions and bounds
  c_i.s = 0;
  c_i.ds = 2.5;
  c_i.dds = 0.0;
  
  c_f.s = 1.0;
  c_f.ds = 0.0;
  c_f.dds = 0;
  
  c_max.ds = 1.0;
  c_max.dds = 100.0;
  c_max.j = 1000.0;
  
  arcTrajGen.setDt( dt );
  arcTrajGen.setInitialConditions( c_i );
  arcTrajGen.setFinalConditions( c_f );
  arcTrajGen.setMaxConditions( c_max );
  
  // compute the entire trajectory
  if ( !arcTrajGen.compute() ) { 
    std::cerr << "Compute failed." << std::endl;
    return 1;
  }
  
  // explore the trajectory
  std::cout << "From " << c_i << "  to " << c_f << " s.t. " << c_max << std::endl;
  std::cout << "Duration : " << arcTrajGen.getDuration() << std::endl;
  for ( double t = 0; t <= arcTrajGen.getDuration() + dt ; t += dt ) {
    arcTrajGen.getConditionsAtTime( t, c_cur );
    std::cout << c_cur << std::endl;
  }
  
  return 0;
}
