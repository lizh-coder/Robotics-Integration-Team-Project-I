//
// Created by stewart on 9/20/20.
//

#include <eigen_conversions/eigen_msg.h>
#include <geometry_msgs/PoseArray.h>
#include <nav_msgs/Odometry.h>
#include <ros/ros.h>
#include <trajectory_msgs/MultiDOFJointTrajectory.h>

#include <mav_trajectory_generation/polynomial_optimization_linear.h>
#include <mav_trajectory_generation/trajectory.h>

#include <tf/transform_datatypes.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <eigen3/Eigen/Dense>

#include <cmath>
#include <algorithm>
#include <vector>
#include <stdexcept>

class WaypointFollower {
  ros::Subscriber currentStateSub;
  ros::Subscriber poseArraySub;
  ros::Publisher desiredStatePub;

  // Current state
  Eigen::Vector3d x;  // current position of the UAV's c.o.m. in the world frame
  bool hasCurrentState = false;
  double currentYaw = 0.0;
  std::vector<Eigen::Vector3d> waypoints;
  std::vector<double> heights, yaws, segmentTimes, gateTimes;
  double virtualTime = 0.0, clockRate = 1.0;
  ros::Time lastPublishTime;
  size_t nextGate = 0;
  double maxSpeed = 8.0, maxAcceleration = 8.5, gateSpeed = 5.6;
  double maxVerticalSpeed = 2.0, maxVerticalAcceleration = 3.0;
  double maxYawRate = 1.8;

  ros::Timer desiredStateTimer;

  ros::Time trajectoryStartTime;
  mav_trajectory_generation::Trajectory trajectory;
  mav_trajectory_generation::Trajectory yaw_trajectory;

  void onCurrentState(nav_msgs::Odometry const& cur_state) {
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  PART 1.1 |  16.485 - Fall 2021  - Lab 4 coding assignment (5 pts)
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // ~
    //
    //  Populate the variable x, which encodes the current world position of the
    //  UAV
    // ~~~~ begin solution
    x << cur_state.pose.pose.position.x,
         cur_state.pose.pose.position.y,
         cur_state.pose.pose.position.z;
    hasCurrentState = true;
    currentYaw = tf::getYaw(cur_state.pose.pose.orientation);
    // ~~~~ end solution
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // ~
    //                                 end part 1.1
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  }

  void generateOptimizedTrajectory(geometry_msgs::PoseArray const& poseArray) {
    if (poseArray.poses.size() < 1) {
      ROS_ERROR("Must have at least one pose to generate trajectory!");
      trajectory.clear();
      yaw_trajectory.clear();
      return;
    }

    if (!hasCurrentState) {
      ROS_WARN_THROTTLE(1, "Waiting for /current_state before generating trajectory.");
      return;
    }

    if (!trajectory.empty()) return;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  PART 1.2 |  16.485 - Fall 2021  - Lab 4 coding assignment (35 pts)
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // ~
    //
    //  We are using the mav_trajectory_generation library
    //  (https://github.com/ethz-asl/mav_trajectory_generation) to perform
    //  trajectory optimization given the waypoints (based on the position and
    //  orientation of the gates on the race course).
    //  We will be finding the trajectory for the position and the trajectory
    //  for the yaw in a decoupled manner.
    //  In this section:
    //  1. Fill in the correct number for D, the dimension we should apply to
    //  the solver to find the positional trajectory
    //  2. Correctly populate the Vertex::Vector structure below (vertices,
    //  yaw_vertices) using the position of the waypoints and the yaw of the
    //  waypoints respectively
    //
    //  Hints:
    //  1. Use vertex.addConstraint(POSITION, position) where position is of
    //  type Eigen::Vector3d to enforce a waypoint position.
    //  2. Use vertex.addConstraint(ORIENTATION, yaw) where yaw is a double
    //  to enforce a waypoint yaw.
    //  3. Remember angle wraps around 2 pi. Be careful!
    //  4. For the ending waypoint for position use .makeStartOrEnd as seen with
    //  the starting waypoint instead of .addConstraint as you would do for the
    //  other waypoints.
    //
    // ~~~~ begin solution

    const int D = 3;  // dimension of each vertex in the trajectory
    mav_trajectory_generation::Vertex start_position(D), end_position(D);
    mav_trajectory_generation::Vertex::Vector vertices;
    mav_trajectory_generation::Vertex start_yaw(1);
    mav_trajectory_generation::Vertex::Vector yaw_vertices;

    // ============================================
    // Convert the pose array to a list of vertices
    // ============================================

    // Start from the measured pose; a zero-yaw start would abruptly spin a
    // vehicle spawned with a different heading.
    using namespace mav_trajectory_generation::derivative_order;
    start_position.makeStartOrEnd(x, SNAP);
    vertices.push_back(start_position);
    start_yaw.makeStartOrEnd(currentYaw, SNAP);
    yaw_vertices.push_back(start_yaw);
    waypoints.clear(); heights.clear(); yaws.clear();
    waypoints.push_back(x); heights.push_back(x.z()); yaws.push_back(currentYaw);

    constexpr double kPi = 3.14159265358979323846;
    double last_yaw = currentYaw;
    for (size_t i = 0; i < poseArray.poses.size(); ++i) {
      // Populate vertices (for the waypoint positions)
      const geometry_msgs::Pose& waypoint = poseArray.poses[i];
      const Eigen::Vector3d position(waypoint.position.x,
                                     waypoint.position.y,
                                     waypoint.position.z);
      {
        mav_trajectory_generation::Vertex vertex(D);
        vertex.addConstraint(POSITION, position);
        // Cross a gate along its normal instead of cutting diagonally through
        // its frame.  Retain nonzero speed so intermediate gates are fly-through.
        Eigen::Vector3d tangent(std::cos(tf::getYaw(waypoint.orientation)),
                                std::sin(tf::getYaw(waypoint.orientation)), 0);
        const Eigen::Vector3d previous = waypoints.back();
        if (tangent.dot(position - previous) < 0) tangent = -tangent;
        // Finish across the final plane before braking to hover behind it.
        // Stopping exactly at its center delays an actual crossing and leaves
        // the vehicle sitting in the aperture.
        const double crossing_speed = i + 1 == poseArray.poses.size() ?
            std::min(3.0, gateSpeed) : gateSpeed;
        vertex.addConstraint(VELOCITY, crossing_speed * tangent);
        // Let the optimizer carry continuous lateral acceleration through
        // each gate rather than forcing a straight, zero-acceleration instant.
        vertices.push_back(vertex);
      }

      // Populate yaw_vertices (for the waypoint yaw angles)
      double yaw = tf::getYaw(waypoint.orientation);
      while (yaw - last_yaw > kPi) yaw -= 2.0 * kPi;
      while (yaw - last_yaw < -kPi) yaw += 2.0 * kPi;
      last_yaw = yaw;

      mav_trajectory_generation::Vertex yaw_vertex(1);
      yaw_vertex.addConstraint(ORIENTATION, yaw);
      yaw_vertices.push_back(yaw_vertex);
      waypoints.push_back(position);
      heights.push_back(position.z());
      yaws.push_back(yaw);
    }

    const geometry_msgs::Pose& final_gate = poseArray.poses.back();
    Eigen::Vector3d exit_direction(std::cos(tf::getYaw(final_gate.orientation)),
                                   std::sin(tf::getYaw(final_gate.orientation)), 0);
    if (exit_direction.dot(waypoints.back() - waypoints[waypoints.size()-2]) < 0)
      exit_direction = -exit_direction;
    const Eigen::Vector3d exit_position = waypoints.back() + 1.5 * exit_direction;
    end_position.makeStartOrEnd(exit_position, SNAP);
    vertices.push_back(end_position);
    mav_trajectory_generation::Vertex end_yaw(1);
    end_yaw.makeStartOrEnd(last_yaw, SNAP);
    yaw_vertices.push_back(end_yaw);
    waypoints.push_back(exit_position);
    heights.push_back(exit_position.z());
    yaws.push_back(last_yaw);

    // ~~~~ end solution
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // ~
    //                                 end part 1.2
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // ============================================================
    // Estimate the time to complete each segment of the trajectory
    // ============================================================

    // HINT: play with these segment times and see if you can finish
    // the race course faster!
    std::vector<double> segment_times;
    // Segment-time estimation alone does not enforce real speed/acceleration
    // limits.  Validate the solved reference and retime it below.
    segment_times.resize(vertices.size()-1);
    for (size_t i = 0; i < segment_times.size(); ++i) {
      const double distance = (waypoints[i+1] - waypoints[i]).norm();
      segment_times[i] = std::max(0.6, 1.08 * distance / gateSpeed);
      if (i == 0 || i+1 == segment_times.size())
        segment_times[i] = std::max(segment_times[i],
            std::sqrt(5.8 * distance / maxAcceleration));
      segment_times[i] = std::max(segment_times[i],
          1.875 * std::abs(heights[i+1] - heights[i]) / maxVerticalSpeed);
      segment_times[i] = std::max(segment_times[i],
          1.875 * std::abs(yaws[i+1] - yaws[i]) / maxYawRate);
    }

    // =====================================================
    // Solve for the optimized trajectory (linear optimizer)
    // =====================================================
    // Position
    const int N = 10;
    // Stretch only the segments that exceed limits. A hard first climb must
    // not slow all eleven subsequent fly-through segments by the same factor.
    for (int iteration = 0; iteration < 10; ++iteration) {
      mav_trajectory_generation::PolynomialOptimization<N> opt(D);
      opt.setupFromVertices(vertices, segment_times, SNAP);
      opt.solveLinear();
      opt.getTrajectory(&trajectory);
      segmentTimes = segment_times;
      std::vector<double> factors(segment_times.size(), 1.0);
      double begin = 0;
      double worst = 1.0;
      for (size_t i = 0; i < segment_times.size(); ++i) {
        const int samples = std::max(40, static_cast<int>(segment_times[i]/0.005));
        for (int sample = 0; sample <= samples; ++sample) {
          const double t = std::min(trajectory.getMaxTime(),
              begin + segment_times[i]*sample/samples);
          Eigen::Vector3d p, v, a;
          double yaw, yaw_rate;
          evaluateReference(t, p, v, a, yaw, yaw_rate);
          double factor = std::max(v.norm()/maxSpeed,
                                  std::sqrt(a.norm()/maxAcceleration));
          factor = std::max(factor, std::abs(v.z())/maxVerticalSpeed);
          factor = std::max(factor, std::sqrt(std::abs(a.z())/maxVerticalAcceleration));
          factor = std::max(factor, std::abs(yaw_rate)/maxYawRate);
          factors[i] = std::max(factors[i], factor);
        }
        begin += segment_times[i];
        worst = std::max(worst, factors[i]);
      }
      if (worst <= 1.015 || iteration == 9) break;
      for (size_t i = 0; i < segment_times.size(); ++i)
        if (factors[i] > 1.015)
          segment_times[i] *= 1.02 * factors[i];
    }

    // Yaw
    mav_trajectory_generation::PolynomialOptimization<N> yaw_opt(1);
    yaw_opt.setupFromVertices(yaw_vertices, segment_times, SNAP);
    yaw_opt.solveLinear();

    // ============================
    // Get the optimized trajectory
    // ============================
    mav_trajectory_generation::Segment::Vector segments;
    //        opt.getSegments(&segments); // Unnecessary?
    yaw_opt.getTrajectory(&yaw_trajectory);
    segmentTimes = segment_times;
    // The height and yaw references below are bounded quintic blends.  Sample
    // the complete reference, including these blends, before enforcing limits.
    double scale = 1.0;
    for (double t = 0; t <= trajectory.getMaxTime(); t += 0.01) {
      Eigen::Vector3d p, v, a;
      double yaw, yaw_rate;
      evaluateReference(t, p, v, a, yaw, yaw_rate);
      scale = std::max(scale, v.norm() / maxSpeed);
      scale = std::max(scale, std::sqrt(a.norm() / maxAcceleration));
      scale = std::max(scale, std::abs(v.z()) / maxVerticalSpeed);
      scale = std::max(scale, std::sqrt(std::abs(a.z()) / maxVerticalAcceleration));
      scale = std::max(scale, std::abs(yaw_rate) / maxYawRate);
    }
    trajectory.scaleSegmentTimes(scale);
    yaw_trajectory.scaleSegmentTimes(scale);
    gateTimes.clear();
    double end_time = 0;
    for (size_t i = 0; i < segmentTimes.size(); ++i) {
      double& duration = segmentTimes[i];
      duration *= scale;
      end_time += duration;
      if (i < poseArray.poses.size()) gateTimes.push_back(end_time);
    }
    virtualTime = 0; clockRate = 1; nextGate = 0;
    lastPublishTime = ros::Time::now();
    trajectoryStartTime = ros::Time::now();

    ROS_INFO("Generated bounded racing trajectory: %zu gates, %.2f s, retime %.2f",
             poseArray.poses.size(), trajectory.getMaxTime(), scale);
  }

  void evaluateReference(double t, Eigen::Vector3d& p, Eigen::Vector3d& v,
                         Eigen::Vector3d& a, double& yaw, double& yaw_rate) {
    using namespace mav_trajectory_generation::derivative_order;
    p = trajectory.evaluate(t, POSITION);
    v = trajectory.evaluate(t, VELOCITY);
    a = trajectory.evaluate(t, ACCELERATION);
    size_t segment = 0;
    double local = t;
    while (segment + 1 < segmentTimes.size() && local > segmentTimes[segment])
      local -= segmentTimes[segment++];
    const double duration = segmentTimes[segment];
    const double s = std::max(0.0, std::min(1.0, local / duration));
    const double blend = s*s*s*(10 + s*(-15 + 6*s));
    const double first = 30*s*s*(1-s)*(1-s) / duration;
    const double second = 60*s*(1-s)*(1-2*s) / (duration*duration);
    const double dz = heights[segment+1] - heights[segment];
    // Monotone and C2: commanded height never goes above the two neighboring
    // gate heights, while velocity/acceleration remain consistent derivatives.
    p.z() = heights[segment] + dz*blend;
    v.z() = dz*first;
    a.z() = dz*second;
    const double dyaw = yaws[segment+1] - yaws[segment];
    yaw = yaws[segment] + dyaw*blend;
    yaw_rate = dyaw*first;
  }

  void publishDesiredState(ros::TimerEvent const& ev) {
    if (trajectory.empty()) return;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  PART 1.3 |  16.485 - Fall 2021  - Lab 4 coding assignment (15 pts)
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // ~
    //
    //  Finally we get to send commands to our controller! First fill in
    //  properly the value for 'nex_point.time_from_start' and 'sampling_time'
    //  (hint: not 0) and after extracting the state information from our
    //  optimized trajectory, finish populating next_point.
    //
    // ~~~~ begin solution
    trajectory_msgs::MultiDOFJointTrajectoryPoint next_point;
    const ros::Time now = ros::Time::now();
    const double dt = std::max(0.0, std::min(0.05, (now - lastPublishTime).toSec()));
    lastPublishTime = now;
    Eigen::Vector3d reference, reference_v, reference_a;
    double reference_yaw, reference_yaw_rate;
    evaluateReference(virtualTime, reference, reference_v, reference_a,
                      reference_yaw, reference_yaw_rate);
    const double error = (x - reference).norm();
    const double target_rate = std::max(0.15, std::min(1.0, (1.0 - error) / 0.6));
    const double old_rate = clockRate;
    clockRate += dt / (0.25 + dt) * (target_rate - clockRate);
    double sampling_time = std::min(trajectory.getMaxTime(), virtualTime + dt*clockRate);
    // Do not advance past an unreached gate merely because time has elapsed.
    if (nextGate < gateTimes.size() && sampling_time >= gateTimes[nextGate]) {
      if ((x - waypoints[nextGate+1]).norm() > 0.65) {
        sampling_time = gateTimes[nextGate];
        clockRate = 0;
        ROS_WARN_THROTTLE(2.0, "Waiting at gate %zu for the vehicle to catch up.", nextGate+1);
      } else {
        ++nextGate;
      }
    }
    virtualTime = sampling_time;
    next_point.time_from_start = ros::Duration(sampling_time);

    // Getting the desired state based on the optimized trajectory we found.
    using namespace mav_trajectory_generation::derivative_order;
    Eigen::Vector3d des_position, des_velocity, des_accel;
    double des_yaw, des_yaw_rate;
    evaluateReference(sampling_time, des_position, des_velocity, des_accel,
                      des_yaw, des_yaw_rate);
    const double rate_derivative = dt > 0 && clockRate > 0 ?
        std::max(-1.0, std::min(1.0, (clockRate - old_rate)/dt)) : 0;
    des_accel = clockRate*clockRate*des_accel + rate_derivative*des_velocity;
    des_velocity *= clockRate;
    ROS_INFO_THROTTLE(1.0,
                      "Traversed %f percent of the trajectory.",
                      sampling_time / trajectory.getMaxTime() * 100);

    // Populate next_point
    next_point.transforms.resize(1);
    next_point.velocities.resize(1);
    next_point.accelerations.resize(1);

    next_point.transforms[0].translation.x = des_position.x();
    next_point.transforms[0].translation.y = des_position.y();
    next_point.transforms[0].translation.z = des_position.z();
    const tf::Quaternion orientation =
        tf::createQuaternionFromYaw(des_yaw);
    next_point.transforms[0].rotation.x = orientation.x();
    next_point.transforms[0].rotation.y = orientation.y();
    next_point.transforms[0].rotation.z = orientation.z();
    next_point.transforms[0].rotation.w = orientation.w();

    next_point.velocities[0].linear.x = des_velocity.x();
    next_point.velocities[0].linear.y = des_velocity.y();
    next_point.velocities[0].linear.z = des_velocity.z();
    next_point.velocities[0].angular.x = 0.0;
    next_point.velocities[0].angular.y = 0.0;
    next_point.velocities[0].angular.z = des_yaw_rate*clockRate;

    next_point.accelerations[0].linear.x = des_accel.x();
    next_point.accelerations[0].linear.y = des_accel.y();
    next_point.accelerations[0].linear.z = des_accel.z();
    next_point.accelerations[0].angular.x = 0.0;
    next_point.accelerations[0].angular.y = 0.0;
    next_point.accelerations[0].angular.z = 0.0;

    desiredStatePub.publish(next_point);

    // ~~~~ end solution
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // ~
    //                                 end part 1.3
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  }

 public:
  explicit WaypointFollower(ros::NodeHandle& nh) {
    ros::NodeHandle private_nh("~");
    private_nh.param("max_speed", maxSpeed, maxSpeed);
    private_nh.param("max_acceleration", maxAcceleration, maxAcceleration);
    private_nh.param("gate_speed", gateSpeed, gateSpeed);
    private_nh.param("max_vertical_speed", maxVerticalSpeed, maxVerticalSpeed);
    private_nh.param("max_vertical_acceleration", maxVerticalAcceleration,
                     maxVerticalAcceleration);
    private_nh.param("max_yaw_rate", maxYawRate, maxYawRate);
    if (maxSpeed <= 0 || maxAcceleration <= 0 || gateSpeed <= 0 ||
        maxVerticalSpeed <= 0 || maxVerticalAcceleration <= 0 || maxYawRate <= 0)
      throw std::invalid_argument("Racing limits must be positive.");
    currentStateSub = nh.subscribe(
        "/current_state", 1, &WaypointFollower::onCurrentState, this);
    poseArraySub = nh.subscribe("/desired_traj_vertices",
                                1,
                                &WaypointFollower::generateOptimizedTrajectory,
                                this);
    desiredStatePub =
        nh.advertise<trajectory_msgs::MultiDOFJointTrajectoryPoint>(
            "/desired_state", 1);
    // Publishing at 5 Hz makes successive desired positions up to 1.6 m apart
    // with the original speed limits, which is too coarse for the controller.
    desiredStateTimer = nh.createTimer(
        ros::Duration(0.01), &WaypointFollower::publishDesiredState, this);
    desiredStateTimer.start();
  }
};

int main(int argc, char** argv) {
  ros::init(argc, argv, "trajectory_generation_node");
  ros::NodeHandle nh;

  WaypointFollower waypointFollower(nh);

  ros::spin();
  return 0;
}
