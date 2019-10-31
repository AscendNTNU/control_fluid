#ifndef FLUID_PATH_H
#define FLUID_PATH_H

#include <vector>
#include <ros/ros.h>

#include <geometry_msgs/Point.h>
#include <ascend_msgs/SplineService.h>

namespace fluid {

    struct PathPoint {
        const double x, y, speed, yaw, curvature;
    };

    struct PathPointResult {
        const PathPoint path_point;
        const double error;
    };

    class Path {

        private:

            double clampAngle(double angle) const;

            std::vector<double> calculateSpeedProfile(const std::vector<double>& yaws, const double& target_speed);

            std::vector<fluid::PathPoint> path_points;

        public:

            Path(const double& target_speed);

            PathPointResult calculateNearestPathPoint(const geometry_msgs::Point& position) const;

            std::vector<fluid::PathPoint> getPathPoints() const;

    };
}

#endif