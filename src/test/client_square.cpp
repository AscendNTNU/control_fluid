#include <ros/ros.h>
#include <geometry_msgs/Pose.h>
#include <fluid/client.h>
#include <fluid/state_identifier.h>

#include <random>


int main(int argc, char** argv) {

    ros::init(argc, argv, "client_square");
    ros::NodeHandle nh;

    fluid::Client client("", 180);

    mavros_msgs::PositionTarget setpoint;
    setpoint.position.x = 0;
    setpoint.position.z = 1.0;

    client.requestTakeOff(0.8, [&](bool completed) {
	/*client.requestMove(setpoint, [&] (bool completed) {
		setpoint.position.y = 1;
		client.requestMove(setpoint, [&] (bool completed) {
			setpoint.position.x = 0;
			setpoint.position.y = 1;
			client.requestMove(setpoint, [&] (bool completed) {
				setpoint.position.x = 0;
				setpoint.position.y = 0;
				client.requestOperationToState(fluid::StateIdentifier::Land, setpoint, [&] (bool completed) {});
			});
		});
	});*/
    });


    ros::Rate rate(1);

    while (ros::ok()) {
            ros::spinOnce();
            rate.sleep();
    }


    return 0;





























/*


    geometry_msgs::Pose pose;
    bool initialized = false;
    float height = 1.0;
    float startX = 0.0;
    float startY = 0.0;


    ros::Rate wait_rate(20);

    while (ros::ok() && !initialized) {
        ros::spinOnce();
        wait_rate.sleep();
    }


    // Just for demonstration, this will make the drone move in straight lines to form a square. When the current move
    // is finished, the next will execute as one can see in the callback.
    // 
    float distance = 5;

    pose.position.x = startX + distance;
    pose.position.y = startY;
    pose.position.z = height;
    
    */
    /*
    client.requestOperation(fluid::OperationIdentifier::Move, pose, [&](bool completed) {
        if (completed) {
                pose.position.x = startX + distance;
                pose.position.y = startY + distance;
                pose.position.z = height;

                operation_client.requestOperation(fluid::OperationIdentifier::MoveOriented, pose, [&](bool completed) {
                    if (completed) {

                        pose.position.x = startX;
                        pose.position.y = startY + distance;
                        pose.position.z = height;

                        operation_client.requestOperation(fluid::OperationIdentifier::MoveOriented, pose, [&](bool completed) {
                            if (completed) {
                                pose.position.x = startX;
                                pose.position.y = startY;
                                pose.position.z = height;

                                operation_client.requestOperation(fluid::OperationIdentifier::MoveOriented, pose, [&](bool completed) {
                                    if (completed) {
                                        geometry_msgs::Pose land_pose;
                                        land_pose.position.x = startX;
                                        land_pose.position.y = startY;
                                        land_pose.position.z = 0;

                                        operation_client.requestOperation(fluid::OperationIdentifier::Land, land_pose, [&](bool completed) {});

                                    }
                                });
                            }
                        });
                    }
                });
        }
    });*/

    return 0;
}
