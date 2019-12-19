#ifndef FLUID_CONTROLLER_H
#define FLUID_CONTROLLER_H

#include <ros/ros.h>
#include <memory>
#include <vector>
#include <string>
#include <mavros_msgs/PositionTarget.h>
#include <ascend_msgs/Spline.h>

#include "trajectory.h"
#include "PID.h"

namespace fluid {

    struct ControllerConfig {
        double kp, ki, kd;
        double target_speed;
        double curvature_gain;
    };

    enum class ControllerType {
        Racing, Exploration
    };

    class Controller {

        public:

            PID pid;

            Controller(const PID& pid);

            mavros_msgs::PositionTarget getSetpoint(const Trajectory& trajectory,
                                                    const geometry_msgs::Pose& pose, 
                                                    const geometry_msgs::Twist& twist, 
                                                    const double& delta_time,
                                                    double& out_error, 
                                                    TrajectoryPoint& out_following_trajectory_point);
    };
}

#endif