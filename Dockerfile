FROM ghcr.io/ascendntnu/ascend-ros-base:8ee8edb
ENV ROS_PACKAGE_NAME=fluid

## Install geographiclib and fetch models
RUN apt-get install -y --no-install-recommends geographiclib-tools && \
        geographiclib-get-geoids minimal

## Install other required packages
RUN apt-get install -y --no-install-recommends \
        ros-${ROS_DISTRO}-mavros \
        ros-${ROS_DISTRO}-mavlink \
        ros-${ROS_DISTRO}-tf \
        ros-${ROS_DISTRO}-tf2-geometry-msgs

# Copy the package files to src/
COPY . ${ROS_WORKSPACE_PATH}/src/${ROS_PACKAGE_NAME}/

# Build the package
RUN /ros_entrypoint.sh catkin_make

ENV LAUNCH=simulator.launch
CMD roslaunch ${ROS_PACKAGE_NAME} ${LAUNCH}
