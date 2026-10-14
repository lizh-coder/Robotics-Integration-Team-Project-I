#include <geometry_msgs/PoseArray.h>
#include <ros/ros.h>
#include <std_msgs/String.h>
#include <tf/transform_broadcaster.h>
#include <trajectory_msgs/MultiDOFJointTrajectory.h>
#include <trajectory_msgs/MultiDOFJointTrajectoryPoint.h>
#include <iostream>
#include <sstream>

#include <math.h>

ros::Subscriber desire_traj_vertices_sub;
ros::Publisher desired_state_pub;
tf::TransformBroadcaster* br;

/**
 * Callback function for listening to the desired vertices msgs
 */
void trajCB(const geometry_msgs::PoseArray& traj_msg) {
  // sanity check for traj_msg size
  if (traj_msg.poses.size() == 0) {
    ROS_ERROR_THROTTLE(1, "Empty trajectory vertices msg.");
    return;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //  PART 0 |  16.485 - Fall 2020  - Lab 4 coding assignment  (10 pts)
  // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
  //
  //  As a simple warm up exercise before we get to the actual 'real deal',
  //  let's just make our quadcopter fly to the first gate in the course.
  //  In this section:
  //   1. Extract the first vertex of the trajectory
  //   2. Set the acceleration and velocities to zero
  //   3. Publish the desired MultiDOFJointTrajectoryPoint
  //   4. Create and publish TF transform of the desired pose
  // ~~~~ begin solution
  const geometry_msgs::Pose& first_gate = traj_msg.poses.front();

  trajectory_msgs::MultiDOFJointTrajectoryPoint desired_state;
  desired_state.time_from_start = ros::Duration(0.0);
  desired_state.transforms.resize(1);
  desired_state.velocities.resize(1);
  desired_state.accelerations.resize(1);

  desired_state.transforms[0].translation.x = first_gate.position.x;
  desired_state.transforms[0].translation.y = first_gate.position.y;
  desired_state.transforms[0].translation.z = first_gate.position.z;
  desired_state.transforms[0].rotation = first_gate.orientation;

  desired_state.velocities[0].linear.x = 0.0;
  desired_state.velocities[0].linear.y = 0.0;
  desired_state.velocities[0].linear.z = 0.0;
  desired_state.velocities[0].angular.x = 0.0;
  desired_state.velocities[0].angular.y = 0.0;
  desired_state.velocities[0].angular.z = 0.0;
  desired_state.accelerations[0].linear.x = 0.0;
  desired_state.accelerations[0].linear.y = 0.0;
  desired_state.accelerations[0].linear.z = 0.0;
  desired_state.accelerations[0].angular.x = 0.0;
  desired_state.accelerations[0].angular.y = 0.0;
  desired_state.accelerations[0].angular.z = 0.0;

  desired_state_pub.publish(desired_state);

  tf::Transform desired_pose;
  desired_pose.setOrigin(tf::Vector3(first_gate.position.x,
                                     first_gate.position.y,
                                     first_gate.position.z));
  desired_pose.setRotation(tf::Quaternion(first_gate.orientation.x,
                                          first_gate.orientation.y,
                                          first_gate.orientation.z,
                                          first_gate.orientation.w));
  br->sendTransform(tf::StampedTransform(desired_pose,
                                         ros::Time::now(),
                                         "world",
                                         "av-desired"));

  // ~~~~ end solution
  // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
  //                                 end part 0
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "simple_traj_planner");
  ros::NodeHandle n;
  ros::Rate loop_rate(500);
  ros::Time start(ros::Time::now());

  // deired traj vertices subscriber
  desire_traj_vertices_sub = n.subscribe("desired_traj_vertices", 10, trajCB);

  // publisher for desired states for controller
  desired_state_pub =
      n.advertise<trajectory_msgs::MultiDOFJointTrajectoryPoint>(
          "desired_state", 1);

  br = new tf::TransformBroadcaster();

  ros::spin();
}
