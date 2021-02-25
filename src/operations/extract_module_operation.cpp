/**
 * @file extract_module_operation.cpp
 */
#include "extract_module_operation.h"

#include "mavros_interface.h"
#include "util.h"
#include "fluid.h" //to get access to the tick rate
#include "type_mask.h"

#include <std_srvs/SetBool.h>

//includes to write in a file
#include <iostream>
#include <fstream>
#include <unistd.h> //to get the current directory

//A list of parameters for the user
#define SAVE_DATA   true
#define SHOW_PRINTS false
#define SAVE_Z      true
#define USE_SQRT    false
#define ATTITUDE_CONTROL 4   //4 = ignore yaw rate   //Attitude control does not work without thrust
#define GROUND_TRUTH true

#define TIME_TO_COMPLETION 0.5 //time in second during which we want the drone to succeed a state before moving to the other.


// LQR tuning
#define POW          1.0        // A factor that multiplies each LQR gains
#define RATION       2.84       // =0.37 / 0.13
#define LQR_POS_GAIN 0.4189     //tuned for 0.13m pitch radius and 0.37m roll radius
#define LQR_VEL_GAIN 1.1062
#define ACCEL_FEEDFORWARD_X 0.0
#define ACCEL_FEEDFORWARD_Y 0.0
const float K_LQR_X[2] = {POW*LQR_POS_GAIN, POW*LQR_VEL_GAIN};
const float K_LQR_Y[2] = {RATION*POW*LQR_POS_GAIN, RATION*POW*LQR_VEL_GAIN};

//smooth transition
#define MAX_VEL     0.14
#define MAX_ACCEL   0.07

#define MAX_ANGLE   400 // in centi-degrees


#if SAVE_DATA
//files saved in home directory
const std::string reference_state_path = std::string(get_current_dir_name()) + "/../reference_state.txt";
const std::string drone_pose_path      = std::string(get_current_dir_name()) + "/../drone_state.txt";    
const std::string drone_setpoints_path = std::string(get_current_dir_name()) + "/../drone_setpoints.txt";
std::ofstream reference_state_f; 
std::ofstream drone_pose_f; 
std::ofstream drone_setpoints_f; 
#endif

uint8_t time_cout = 0; //used not to do some stuffs at every tick

//function called when creating the operation
ExtractModuleOperation::ExtractModuleOperation(const float& fixed_mast_yaw, const float& offset) : 
            Operation(OperationIdentifier::EXTRACT_MODULE, false, false), fixed_mast_yaw(fixed_mast_yaw) 
    { 
        //Choose an initial offset. It is the offset for the approaching state.
        //the offset is set in the frame of the mast:    
        desired_offset.x = offset;     //forward
        desired_offset.y = 0.0;     //left
        desired_offset.z = -0.5;    //up

    }

void ExtractModuleOperation::initialize() {
    #if GROUND_TRUTH
    module_pose_subscriber = node_handle.subscribe("/simulator/module/ground_truth/pose",
                                     10, &ExtractModuleOperation::modulePoseCallback, this);
    #else
    module_pose_subscriber = node_handle.subscribe("/simulator/module/noisy/pose",
                                     10, &ExtractModuleOperation::modulePoseCallback, this);
    #endif
    // backpropeller_client = node_handle.serviceClient<std_srvs::SetBool>("/airsim/backpropeller");

    attitude_pub = node_handle.advertise<mavros_msgs::AttitudeTarget>("/mavros/setpoint_raw/attitude",10);
    //creating own publisher to choose exactly when we send messages
    altitude_and_yaw_pub = node_handle.advertise<mavros_msgs::PositionTarget>("/mavros/setpoint_raw/local",10);
    attitude_setpoint.type_mask = ATTITUDE_CONTROL;   
    
    setpoint.type_mask = TypeMask::POSITION_AND_VELOCITY;

    MavrosInterface mavros_interface;
    mavros_interface.setParam("ANGLE_MAX", MAX_ANGLE);
    ROS_INFO_STREAM(ros::this_node::getName().c_str() << ": Sat max angle to: " << MAX_ANGLE/100.0 << " deg.");

    // The desired offset and the transition state are mesured in the mast frame
    transition_state.state.position = desired_offset;
    //transition_state.state.position.y = desired_offset.y;
    //transition_state.state.position.z = desired_offset.z;

    //At the beginning we are far from the mast, we can safely so do a fast transion.
    //But transition state is also the offset, so it should be useless.
    transition_state.cte_acc = 3*MAX_ACCEL; 
    transition_state.max_vel = 3*MAX_VEL;
    completion_count =0;

    //get the gains from the launch file.
    const float* temp = Fluid::getInstance().configuration.LQR_gains;
    for (uint8_t i=0 ; i<4 ; i++) { LQR_gains[i] = temp[i];}
    ROS_INFO_STREAM("GOT LQR GAINS: "<< LQR_gains[0] << " ; " << LQR_gains[1]);
    
    #if SAVE_DATA
    //create a header for the datafiles.
    initLog(reference_state_path); 
    initLog(drone_pose_path); 
    initLog(drone_setpoints_path); 
    #endif
}

bool ExtractModuleOperation::hasFinishedExecution() const { return extraction_state == ExtractionState::EXTRACTED; }

void ExtractModuleOperation::modulePoseCallback(
    const geometry_msgs::PoseStampedConstPtr module_pose_ptr) {
    previous_module_state = module_state;

    module_state.header = module_pose_ptr->header;
    module_state.position = module_pose_ptr->pose.position;
    module_state.velocity = estimateModuleVel();    
    module_state.acceleration_or_force = estimateModuleAccel();
    //I may want to also extract the mast_yaw
}

#if SAVE_DATA
void ExtractModuleOperation::initLog(const std::string file_name)
{ //create a header for the logfile.
    std::ofstream save_file_f;
     save_file_f.open(file_name);
    if(save_file_f.is_open())
    {
//        ROS_INFO_STREAM(ros::this_node::getName().c_str() << ": " << file_name << " open successfully");
        save_file_f << "Time\tPos.x\tPos.y\tPos.z\tVel.x\tVel.y\tVel.z\tmodule_estimate_vel.x\tmodule_estimate_vel.y\tmodule_estimate_vel.z\n";
        save_file_f.close();
    }
    else
    {
        ROS_INFO_STREAM(ros::this_node::getName().c_str() << "could not open " << file_name);
    }
}

void ExtractModuleOperation::saveLog(const std::string file_name, const geometry_msgs::PoseStamped pose, const geometry_msgs::TwistStamped vel, const geometry_msgs::Vector3 accel)
{
    std::ofstream save_file_f;
    save_file_f.open (file_name, std::ios::app);
    if(save_file_f.is_open())
    {
        save_file_f << std::fixed << std::setprecision(3) //only 3 decimals
                        << ros::Time::now() << "\t"
                        << pose.pose.position.x << "\t"
                        << pose.pose.position.y << "\t"
                        #if SAVE_Z
                        << pose.pose.position.z << "\t"
                        #endif
                        << vel.twist.linear.x << "\t"
                        << vel.twist.linear.y << "\t"
                        #if SAVE_Z
                        << vel.twist.linear.z << "\t"
                        #endif
                        << accel.x << "\t"
                        << accel.y 
                        #if SAVE_Z
                        << "\t" << accel.z
                        #endif
                        << "\n";
        save_file_f.close();
    }
}

void ExtractModuleOperation::saveLog(const std::string file_name, const mavros_msgs::PositionTarget data)
{
    std::ofstream save_file_f;
    save_file_f.open (file_name, std::ios::app);
    if(save_file_f.is_open())
    {
        save_file_f << std::fixed << std::setprecision(3) //only 3 decimals
                        << ros::Time::now() << "\t"
                        << data.position.x << "\t"
                        << data.position.y << "\t"
                        #if SAVE_Z
                        << data.position.z << "\t"
                        #endif
                        << data.velocity.x << "\t"
                        << data.velocity.y << "\t"
                        #if SAVE_Z
                        << data.velocity.z << "\t"
                        #endif
                        << data.acceleration_or_force.x << "\t"
                        << data.acceleration_or_force.y 
                        #if SAVE_Z
                        << "\t" << data.acceleration_or_force.z
                        #endif
                        << "\n";
        save_file_f.close();
    }
}
#endif

geometry_msgs::Vector3 ExtractModuleOperation::estimateModuleVel(){
    // estimate the velocity of the module by a simple derivation of the position.
    // In the long run, I expect to receive a nicer estimate by perception or to create a KF myself.
    geometry_msgs::Vector3 vel;
    double dt = (module_state.header.stamp - previous_module_state.header.stamp).nsec/1000000000.0;
    vel.x = (module_state.position.x - previous_module_state.position.x)/dt;
    vel.y = (module_state.position.y - previous_module_state.position.y)/dt;
    vel.z = (module_state.position.z - previous_module_state.position.z)/dt;
    return vel;
}

geometry_msgs::Vector3 ExtractModuleOperation::estimateModuleAccel(){
    // estimate the acceleration of the module by simply derivating the velocity.
    // In the long run, I expect to receive a nicer estimate by perception or to createa KF myself.
    geometry_msgs::Vector3 Accel;
    double dt = (module_state.header.stamp - previous_module_state.header.stamp).nsec/1000000000.0;
    Accel.x = (module_state.velocity.x - previous_module_state.velocity.x)/dt;
    Accel.y = (module_state.velocity.y - previous_module_state.velocity.y)/dt;
    Accel.z = (module_state.velocity.z - previous_module_state.velocity.z)/dt;
    return Accel;
}

mavros_msgs::PositionTarget ExtractModuleOperation::rotate(mavros_msgs::PositionTarget setpoint, float yaw){
    mavros_msgs::PositionTarget rotated_setpoint;
    rotated_setpoint.position = rotate(setpoint.position);
    rotated_setpoint.velocity = rotate(setpoint.velocity);
    rotated_setpoint.acceleration_or_force = rotate(setpoint.acceleration_or_force);

    return rotated_setpoint;
}

geometry_msgs::Vector3 ExtractModuleOperation::rotate(geometry_msgs::Vector3 pt, float yaw){
    geometry_msgs::Vector3 rotated_point;
    rotated_point.x = cos(fixed_mast_yaw) * pt.x - sin(fixed_mast_yaw) * pt.y;
    rotated_point.y = cos(fixed_mast_yaw) * pt.y + sin(fixed_mast_yaw) * pt.x;
    rotated_point.z = pt.z;

    return rotated_point;
}

geometry_msgs::Point ExtractModuleOperation::rotate(geometry_msgs::Point pt, float yaw){
    geometry_msgs::Point rotated_point;
    rotated_point.x = cos(fixed_mast_yaw) * pt.x - sin(fixed_mast_yaw) * pt.y;
    rotated_point.y = cos(fixed_mast_yaw) * pt.y + sin(fixed_mast_yaw) * pt.x;
    rotated_point.z = pt.z;

    return rotated_point;
}

geometry_msgs::Vector3 ExtractModuleOperation::LQR_to_acceleration(mavros_msgs::PositionTarget ref){
    accel_target.z = 0;
    #if USE_SQRT
        accel_target.x = K_LQR_X[0] * Util::signed_sqrt(ref.position.x - getCurrentPose().pose.position.x) 
                     + K_LQR_X[1] * Util::signed_sqrt(ref.velocity.x - getCurrentTwist().twist.linear.x) 
                     + ACCEL_FEEDFORWARD_X * ref.acceleration_or_force.x;
        accel_target.y = K_LQR_Y[0] * Util::signed_sqrt(ref.position.y - getCurrentPose().pose.position.y) 
                     + K_LQR_Y[1] * Util::signed_sqrt(ref.velocity.y - getCurrentTwist().twist.linear.y) 
                     + ACCEL_FEEDFORWARD_X * ref.acceleration_or_force.y;

    #else
        accel_target.x = K_LQR_X[0] * (ref.position.x - getCurrentPose().pose.position.x) 
                     + K_LQR_X[1] * (ref.velocity.x - getCurrentTwist().twist.linear.x) 
                     + ACCEL_FEEDFORWARD_X * ref.acceleration_or_force.x;
        accel_target.y = K_LQR_Y[0] * (ref.position.y - getCurrentPose().pose.position.y) 
                     + K_LQR_Y[1] * (ref.velocity.y - getCurrentTwist().twist.linear.y) 
                     + ACCEL_FEEDFORWARD_X * ref.acceleration_or_force.y;
    #endif
    // the right of the mast is the left of the drone: the drone is facing the mast
    accel_target.x = - accel_target.x;
    return accel_target;
}

geometry_msgs::Quaternion ExtractModuleOperation::accel_to_orientation(geometry_msgs::Vector3 accel){
    double yaw = fixed_mast_yaw + M_PI; //we want to face the mast
    double roll = atan2(accel.y,9.81);
    double pitch = atan2(accel.x,9.81);
    return Util::euler_to_quaternion(yaw, roll, pitch);
}

void ExtractModuleOperation::update_attitude_input(mavros_msgs::PositionTarget offset){
    mavros_msgs::PositionTarget ref = Util::addPositionTarget(module_state, offset);

    attitude_setpoint.header.stamp = ros::Time::now();
    attitude_setpoint.thrust = 0.5; //this is the thrust that allow a constant altitude no matter what

    accel_target = LQR_to_acceleration(ref);
    attitude_setpoint.orientation = accel_to_orientation(accel_target);
}

void ExtractModuleOperation::update_transition_state()
{
// try to make a smooth transition when the relative targeted position between the drone
// and the mast is changed

// Analysis on the x axis
    if (abs(desired_offset.x - transition_state.state.position.x) >= 0.001){
    // if we are in a transition state on the x axis
        transition_state.finished_bitmask &= ~0x1;
        if (Util::sq(transition_state.state.velocity.x) / 2.0 / transition_state.cte_acc 
                            >= abs(desired_offset.x - transition_state.state.position.x))
        {
        // if it is time to brake to avoid overshoot
            //set the transition acceleration (or deceleration) to the one that will lead us to the exact point we want
            transition_state.state.acceleration_or_force.x = - Util::sq(transition_state.state.velocity.x) 
                                            /2.0 / (desired_offset.x - transition_state.state.position.x);
        }
        else if (abs(transition_state.state.velocity.x) > transition_state.max_vel)
        {
        // if we have reached max transitionning speed
            //we stop accelerating and maintain speed
            transition_state.state.acceleration_or_force.x = 0.0;
        }
        else{
        //we are in the acceleration phase of the transition){
            if (desired_offset.x - transition_state.state.position.x > 0.0)
                transition_state.state.acceleration_or_force.x = transition_state.cte_acc;
            else
                transition_state.state.acceleration_or_force.x = - transition_state.cte_acc;
        }
        // Whatever the state we are in, update velocity and position of the target
        transition_state.state.velocity.x = transition_state.state.velocity.x + transition_state.state.acceleration_or_force.x / (float)rate_int;
        transition_state.state.position.x = transition_state.state.position.x + transition_state.state.velocity.x / (float)rate_int;
        
    }
    else if (abs(transition_state.state.velocity.x) < 0.1){
        //setpoint reached destination on this axis
        transition_state.state.position.x = desired_offset.x;
        transition_state.state.velocity.x = 0.0;
        transition_state.state.acceleration_or_force.x = 0.0;
        transition_state.finished_bitmask |= 0x1;
    }

// Analysis on the y axis, same as on the x axis
    if (abs(desired_offset.y - transition_state.state.position.y) >= 0.001){
        transition_state.finished_bitmask = ~0x2;
        if (Util::sq(transition_state.state.velocity.y) / 2.0 / transition_state.cte_acc 
                            >= abs(desired_offset.y - transition_state.state.position.y))
            transition_state.state.acceleration_or_force.y = - Util::sq(transition_state.state.velocity.y) 
                                            /2.0 / (desired_offset.y - transition_state.state.position.y);
        else if (abs(transition_state.state.velocity.y) > transition_state.max_vel)
            transition_state.state.acceleration_or_force.y = 0.0;
        else{
            if (desired_offset.y - transition_state.state.position.y > 0.0)
                transition_state.state.acceleration_or_force.y = transition_state.cte_acc;
            else
                transition_state.state.acceleration_or_force.y = - transition_state.cte_acc;
            }
        transition_state.state.velocity.y  =   transition_state.state.velocity.y  + transition_state.state.acceleration_or_force.y / (float)rate_int;
        transition_state.state.position.y =  transition_state.state.position.y  + transition_state.state.velocity.y / (float)rate_int;
    }
    else if (abs(transition_state.state.velocity.y) < 0.1){
        transition_state.state.position.y = desired_offset.y;
        transition_state.state.velocity.y = 0.0;
        transition_state.state.acceleration_or_force.y = 0.0;
        transition_state.finished_bitmask |= 0x2;
    }

// Analysis on the z axis, same as on the x axis
    if (abs(desired_offset.z - transition_state.state.position.z) >= 0.001){
        transition_state.finished_bitmask = ~0x4;
        if (Util::sq(transition_state.state.velocity.z) / 2.0 / transition_state.cte_acc 
                            >= abs(desired_offset.z - transition_state.state.position.z))
            transition_state.state.acceleration_or_force.z = - Util::sq(transition_state.state.velocity.z) 
                                            /2.0 / (desired_offset.z - transition_state.state.position.z);
        else if (abs(transition_state.state.velocity.z) > transition_state.max_vel)
            transition_state.state.acceleration_or_force.z = 0.0;
        else {
            if (desired_offset.z - transition_state.state.position.z > 0.0)
                transition_state.state.acceleration_or_force.z = transition_state.cte_acc;
            else 
                transition_state.state.acceleration_or_force.z = - transition_state.cte_acc;
        }
        transition_state.state.velocity.z =  transition_state.state.velocity.z + transition_state.state.acceleration_or_force.z / (float)rate_int;
        transition_state.state.position.z =  transition_state.state.position.z + transition_state.state.velocity.z / (float)rate_int;
    }
    else if (abs(transition_state.state.velocity.z) < 0.1){
        transition_state.state.position.z = desired_offset.z;
        transition_state.state.velocity.z = 0.0;
        transition_state.state.acceleration_or_force.z = 0.0;
        transition_state.finished_bitmask |= 0x4;
    }
}

void ExtractModuleOperation::tick() {
    time_cout++;
    // Wait until we get the first module position readings before we do anything else.
    if (module_state.header.seq == 0) {
        if(time_cout%rate_int==0)
            printf("Waiting for callback\n");
        return;
    }
    
    update_transition_state();
    mavros_msgs::PositionTarget smooth_rotated_offset = rotate(transition_state.state,fixed_mast_yaw);

    const double dx = module_state.position.x + smooth_rotated_offset.position.x - getCurrentPose().pose.position.x;
    const double dy = module_state.position.y + smooth_rotated_offset.position.y - getCurrentPose().pose.position.y;
    const double dz = module_state.position.z + smooth_rotated_offset.position.z - getCurrentPose().pose.position.z;
    const double distance_to_reference_with_offset = sqrt(Util::sq(dx) + Util::sq(dy) + Util::sq(dz));
    

    switch (extraction_state) {
        case ExtractionState::APPROACHING: {
            #if SHOW_PRINTS
            if(time_cout%(rate_int*2)==0) printf("APPROACHING\t");
            printf("distance to ref %f\n", distance_to_reference_with_offset);
            #endif
            if (transition_state.finished_bitmask & 0x7 && distance_to_reference_with_offset < 0.07) {
                if (completion_count <= ceil(TIME_TO_COMPLETION*(float) rate_int) )
                    completion_count++;
                else{
                    extraction_state = ExtractionState::OVER;
                    ROS_INFO_STREAM(ros::this_node::getName().c_str()
                                << ": " << "Approaching -> Over");
                    //the offset is set in the frame of the mast:    
                    desired_offset.x = 0.25;  //forward   //right //the distance from the drone to the FaceHugger
                    desired_offset.y = 0.0;   //left   //front
                    desired_offset.z = -0.45;  //up   //up
                    transition_state.cte_acc = MAX_ACCEL;
                    transition_state.max_vel = MAX_VEL;
                    completion_count= 0;
                }
            }
            else
                completion_count = 0;
            break;
        }
        case ExtractionState::OVER: {
            #if SHOW_PRINTS
            if(time_cout%(rate_int*2)==0) printf("OVER\n");
            #endif

            //todo write a smart evalutation function to know when to move to the next state
            if (distance_to_reference_with_offset < 
                    0.04 && std::abs(getCurrentYaw() - fixed_mast_yaw) < M_PI / 50.0) {
                if (completion_count <= ceil(TIME_TO_COMPLETION*(float) rate_int) )
                    completion_count++;
                else{
                    extraction_state = ExtractionState::EXTRACTING;
                    ROS_INFO_STREAM(ros::this_node::getName().c_str()
                                << ": " << "Over -> Extracting");
                    desired_offset.x = 0.25;  //forward   //right //the distance from the drone to the FaceHugger
                    desired_offset.y = 0.0;   //left      //front
                    desired_offset.z = -0.8;  //up        //up
                }
            }
            else
                completion_count = 0;
            break;
        }
        case ExtractionState::EXTRACTING: {
            #if SHOW_PRINTS
            if(time_cout%(rate_int*2)==0) printf("EXTRACTING\n");
            #endif

            //Do something to release the FaceHugger at the righ moment
            /*
            if (!called_backpropeller_service) {
                std_srvs::SetBool request;
                request.request.data = true;
                backpropeller_client.call(request);
                called_backpropeller_service = true;
            } */          

            // If the module is on the way down
            // TODO: this should be checked in a better way
            if (module_state.position.z < 0.5) {
                if (completion_count <= ceil(TIME_TO_COMPLETION*(float) rate_int) )
                    completion_count++;
                else{
                    extraction_state = ExtractionState::EXTRACTED;
                    ROS_INFO_STREAM(ros::this_node::getName().c_str() << "Module extracted!"); 
                    std_srvs::SetBool request;
                    request.request.data = false; 
    //                backpropeller_client.call(request);
    //                called_backpropeller_service = false;

                    //we move backward to ensure there will be no colision
                    // We directly set the transition state as we want to move as fast as possible
                    // and we don't mind anymore about the relative position to the mast
                    transition_state.state.position.x = 1.70;  //forward   //right //the distance from the drone to the FaceHugger
                    transition_state.state.position.y = 0.0;   //left      //front
                    transition_state.state.position.z = -0.8;  //up        //up
                }
            }
            else
                completion_count = 0;
            break;
        }
    }//end switch state

    if (time_cout % rate_int == 0)
    {
    #if SHOW_PRINTS
    //    printf("desired offset \t\tx %f, y %f, z %f\n",desired_offset.x,
    //                    desired_offset.y, desired_offset.z);
        printf("transition state\t x %f, y %f, z %f \tyaw: %f\n",transition_state.state.position.x,
                        transition_state.state.position.y, transition_state.state.position.z,
                        getCurrentYaw());
    //    printf("transition vel\t x %f, y %f, z %f\n",transition_state.state.velocity.x,
    //                    transition_state.state.velocity.y, transition_state.state.velocity.z);
    //    printf("transition accel\t x %f, y %f, z %f\n",transition_state.state.acceleration_or_force.x,
    //                    transition_state.state.acceleration_or_force.y, transition_state.state.acceleration_or_force.z);
    #endif
    }

    update_attitude_input(smooth_rotated_offset);

    if (time_cout % 2 == 0)
    {
        // todo: it may be possible to publish more often without any trouble.
        setpoint.header.stamp = ros::Time::now();
        setpoint.yaw = fixed_mast_yaw+M_PI;
        setpoint.position.x = module_state.position.x + smooth_rotated_offset.position.x;
        setpoint.position.y = module_state.position.y + smooth_rotated_offset.position.y;
        setpoint.position.z = module_state.position.z + smooth_rotated_offset.position.z;
        setpoint.velocity.x = module_state.velocity.x + smooth_rotated_offset.velocity.x;
        setpoint.velocity.y = module_state.velocity.y + smooth_rotated_offset.velocity.y;
        setpoint.velocity.z = module_state.velocity.z + smooth_rotated_offset.velocity.z;

        altitude_and_yaw_pub.publish(setpoint);

    }

    attitude_pub.publish(attitude_setpoint);

    #if SAVE_DATA
    //save the control data into files
    saveLog(reference_state_path,Util::addPositionTarget(module_state, smooth_rotated_offset));
    saveLog(drone_pose_path, getCurrentPose(),getCurrentTwist(),getCurrentAccel());
    saveLog(drone_setpoints_path,smooth_rotated_offset);
    #endif

}
