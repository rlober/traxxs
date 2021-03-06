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

#ifndef TRAXXS_TRACKER_TRACKER_SPACE_H
#define TRAXXS_TRACKER_TRACKER_SPACE_H

#include <traxxs/tracker/core.hpp>

namespace traxxs {
namespace tracker {

/** 
 * \brief An abstract TrackerSpaceValidator interface, allowing to define the validation of a current state w.r.t. a reference state
 */  
class TrackerSpaceValidator
{
 public:
  TrackerSpaceValidator(){};
  virtual ~TrackerSpaceValidator(){};
  
 public: // the non-virtual public interface
  /**
   * \brief validate (or not) a current state w.r.t. a reference state
   * \param[in]   dt the time increment at which the tracker is working
   * \param[in]   current_state the current system state
   * \param[in]   reference_state the reference state used for validation of current state
   */
  virtual TrackerValidatorStatus validate( double dt, const trajectory::TrajectoryState& current_state, const trajectory::TrajectoryState& reference_state ) {
    return this->do_validate( dt, current_state, reference_state );
  };
  
 protected: // the (pure virtual) implementation
  virtual TrackerValidatorStatus do_validate( double dt, const trajectory::TrajectoryState& current_state, const trajectory::TrajectoryState& reference_state ) = 0;
  
};


/** 
 * \brief A Space-pursuit trajectory tracker
 * Simply, the tracker keeps track of the virtual time only if the provided TrackerSpaceValidator authorizes it. 
 * On every call to TrackerSpacePursuit::next(), the virtual time will be incremented,
 * if and only if the provided TrackerSpaceValidator validates the new state w.r.t. the previously output state.
 * by the input dt until the end of the trajectory is met.
 * TrackerSpacePursuit::next() will return traxxs::tracker::TrackerStatus::Stalled if the increment is not validated, and new_state_out will be equal to the previously output state
 */
class TrackerSpacePursuit : public Tracker
{
 public:
  TrackerSpacePursuit( const std::shared_ptr< TrackerSpaceValidator >& validator ) 
      : Tracker(), validator_( validator ) {};
  
 protected:
  virtual TrackerStatus do_next( double dt, const trajectory::TrajectoryState& current_state, trajectory::TrajectoryState& new_state_out ) override {
    bool is_beyond;
    if ( this->trajectory_ == nullptr )
      return TrackerStatus::NotInitialized;
    if ( this->validator_ == nullptr )
      return TrackerStatus::NotInitialized;
    
    if ( this->prev_output_ == nullptr ) {
      trajectory::TrajectoryState init_state;
      this->trajectory_->getState( this->current_virtual_t_, init_state );
      this->prev_output_ = std::make_shared< trajectory::TrajectoryState > ( init_state );
    }
    TrackerValidatorStatus ret_valid = this->validator_->validate( dt, current_state, *prev_output_.get() );
    // test if the validator validates the increment
    if ( ret_valid != TrackerValidatorStatus::Success ) {
      if ( ret_valid == TrackerValidatorStatus::Failure ) { // if failure, we stall !
          new_state_out = *prev_output_.get();
          return TrackerStatus::Stalled;
      } else { // this is neither a success nor a failure, i.e. an error !
        return TrackerStatus::Error;
      }
    }
    bool ret = this->trajectory_->getState( this->current_virtual_t_ + dt, new_state_out, nullptr, &is_beyond );
    if ( !ret )
      return TrackerStatus::Error;
    
    // store it for next validation
    prev_output_ = std::make_shared< trajectory::TrajectoryState > ( new_state_out );
    
    // if beyond
    /** \todo should we check for "is_beyond" BEFORE checking for "stalled" (i.e. validation) ? */
    if ( is_beyond )
      return TrackerStatus::Finished;
    // keep track of the increment
    this->current_virtual_t_ += dt;
    return TrackerStatus::Incremented;
  }
  
 protected:
  std::shared_ptr< TrackerSpaceValidator > validator_ = nullptr;
  std::shared_ptr< trajectory::TrajectoryState > prev_output_ = nullptr;
};

/**
 * Validates a 3D pose using a 4d check: position (3) + angle (1)
 * \warning Expects TrajectoryState::x as a 7d-vector, see traxxs::path::Pose::toVector()
 */
class TrackerSpaceValidatorPose4d: public TrackerSpaceValidator
{
 public:
  TrackerSpaceValidatorPose4d( double position_tolerance, double angle_tolerance ) 
      : TrackerSpaceValidator(), tol_angle_( angle_tolerance ) {
    tol_position_.fill( std::fabs( position_tolerance ) );
  }
  
  TrackerSpaceValidatorPose4d( const Eigen::Vector3d& position_tolerance, double angle_tolerance ) 
      : TrackerSpaceValidator(), tol_angle_( angle_tolerance ), tol_position_( position_tolerance.cwiseAbs() ) {
  }
  
 protected: // the implementation
  virtual TrackerValidatorStatus do_validate( double dt, const trajectory::TrajectoryState& current_state, const trajectory::TrajectoryState& reference_state ) override {
    (void) dt;
    
    try {
      pose_ref_.fromVector( reference_state.x );
      pose_cur_.fromVector( current_state.x );
    } catch ( const std::invalid_argument* e ) {
      return TrackerValidatorStatus::Error;
    }
    
    error_position_ = pose_ref_.p - pose_cur_.p;
    if ( ( error_position_.cwiseAbs() - tol_position_ ).maxCoeff() > 0 )
      return TrackerValidatorStatus::Failure;
    
    double error_angle = Eigen::AngleAxisd( pose_ref_.q * pose_cur_.q.inverse() ).angle();
    if ( std::fabs( error_angle ) > tol_angle_ )
      return TrackerValidatorStatus::Failure;
    
    return TrackerValidatorStatus::Success;
  }
  
 protected:
  double tol_angle_;
  Eigen::Vector3d tol_position_;
 protected: // pre-allocated variables
  path::Pose pose_cur_, pose_ref_;
  Eigen::Vector3d error_position_;
};


} // namespace tracker
} // namespace traxxs

#endif //TRAXXS_TRACKER_TRACKER_SPACE_H
