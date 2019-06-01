//
// Created by simengangstad on 11.10.18.
//


#include "take_off_state.h"
#include "pose_util.h"

bool fluid::TakeOffState::hasFinishedExecution() {
    return PoseUtil::distanceBetween(current_pose_, setpoint) < 0.3 && 
    	   std::abs(getCurrentTwist().twist.linear.x) < 0.05 && 
    	   std::abs(getCurrentTwist().twist.linear.y) < 0.05 && 
    	   std::abs(getCurrentTwist().twist.linear.z) < 0.05;
}

void fluid::TakeOffState::initialize() {
    setpoint.position.x = getCurrentPose().pose.position.x;
    setpoint.position.y = getCurrentPose().pose.position.y;

	if (setpoint.position.z <= 0.1) {
		setpoint.position.z = 1.0;
	}
}

void fluid::TakeOffState::tick() {
    setpoint.type_mask = fluid::TypeMask::Default;
}