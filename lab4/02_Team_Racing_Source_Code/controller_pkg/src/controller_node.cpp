#include <ros/ros.h>

#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/utils.h>
#include <mav_msgs/Actuators.h>
#include <nav_msgs/Odometry.h>
#include <trajectory_msgs/MultiDOFJointTrajectoryPoint.h>
#include <cmath>

#define PI M_PI

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  PART 0 |  16.485 - Fall 2019  - Lab 3 coding assignment
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  In this code, we ask you to implement a geometric controller for a
//  simulated UAV, following the publication:
//
//  [1] Lee, Taeyoung, Melvin Leoky, N. Harris McClamroch. "Geometric tracking
//      control of a quadrotor UAV on SE (3)." Decision and Control (CDC),
//      49th IEEE Conference on. IEEE, 2010
//
//  We use variable names as close as possible to the conventions found in the
//  paper, however, we have slightly different conventions for the aerodynamic
//  coefficients of the propellers (refer to the lecture notes for these).
//  Additionally, watch out for the different conventions on reference frames
//  (see Lab 3 Handout for more details).
//
//  The include below is strongly suggested [but not mandatory if you have
//  better alternatives in mind :)]. Eigen is a C++ library for linear algebra
//  that will help you significantly with the implementation. Check the
//  quick reference page to learn the basics:
//
//  https://eigen.tuxfamily.org/dox/group__QuickRefPage.html

#include <eigen3/Eigen/Dense>

// If you choose to use Eigen, tf2 provides useful functions to convert tf2
// messages to eigen types and vice versa.
#include <tf2_eigen/tf2_eigen.h>

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//                                 end part 0
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class controllerNode{
  ros::NodeHandle nh;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //  PART 1 |  Declare ROS callback handlers
  // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
  //
  // In this section, you need to declare:
  //   1. two subscribers (for the desired and current UAVStates)
  //   2. one publisher (for the propeller speeds)
  //   3. a timer for your main control loop
  //
  // ~~~~ begin solution
  //
 //     **** FILL IN HERE ***
  ros::Subscriber desired_state_subscriber;
  ros::Subscriber current_state_subscriber;
  ros::Publisher motor_command_publisher;
  ros::Timer control_timer;
  //
  // ~~~~ end solution
  // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
  //                                 end part 1
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Controller parameters
  double kx, kv, kr, komega; // controller gains - [1] eq (15), (16)

  // Physical constants (we will set them below)
  double m;              // mass of the UAV
  double g;              // gravity acceleration
  double d;              // distance from the center of propellers to the c.o.m.
  double cf,             // Propeller lift coefficient
         cd;             // Propeller drag coefficient
  Eigen::Matrix3d J;     // Inertia Matrix
  Eigen::Vector3d e3;    // [0,0,1]
  Eigen::MatrixXd F2W;   // Wrench-rotor speeds map

  // Controller internals (you will have to set them below)
  // Current state
  Eigen::Vector3d x;     // current position of the UAV's c.o.m. in the world frame
  Eigen::Vector3d v;     // current velocity of the UAV's c.o.m. in the world frame
  Eigen::Matrix3d R;     // current orientation of the UAV
  Eigen::Vector3d omega; // current angular velocity of the UAV's c.o.m. in the *body* frame

  // Desired state
  Eigen::Vector3d xd;    // desired position of the UAV's c.o.m. in the world frame
  Eigen::Vector3d vd;    // desired velocity of the UAV's c.o.m. in the world frame
  Eigen::Vector3d ad;    // desired acceleration of the UAV's c.o.m. in the world frame
  double yawd;           // desired yaw angle

  // The control loop starts before the bridge has necessarily delivered an
  // odometry or trajectory message.  Never treat the zero-initialized vectors
  // as a real command: until a trajectory arrives, hold the first measured
  // pose instead.
  bool has_current_state = false;
  bool has_desired_state = false;
  bool racing_safety = false;
  double racing_tilt_limit = 0.82;
  double racing_rate_limit = 3.0;
  Eigen::Matrix3d previous_Rd = Eigen::Matrix3d::Identity();
  Eigen::Vector3d desired_omega = Eigen::Vector3d::Zero();
  ros::Time previous_control_time;

  double hz;             // frequency of the main control loop


  static Eigen::Vector3d Vee(const Eigen::Matrix3d& in){
    Eigen::Vector3d out;
    out << in(2,1), in(0,2), in(1,0);
    return out;
  }

  static double signed_sqrt(double val){
    return val>0?sqrt(val):-sqrt(-val);
  }

public:
  controllerNode():e3(0,0,1),F2W(4,4),hz(1000.0){

      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //  PART 2 |  Initialize ROS callback handlers
      // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
      //
      // In this section, you need to initialize your handlers from part 1.
      // Specifically:
      //  - bind controllerNode::onDesiredState() to the topic "desired_state"
      //  - bind controllerNode::onCurrentState() to the topic "current_state"
      //  - bind controllerNode::controlLoop() to the created timer, at frequency
      //    given by the "hz" variable
      //
      // Hints: 
      //  - use the nh variable already available as a class member
      //  - read the lab 3 handout to fnd the message type
      //
      // ~~~~ begin solution
      //
     //     **** FILL IN HERE ***
      desired_state_subscriber =
          nh.subscribe("desired_state", 1,
                       &controllerNode::onDesiredState, this);

      current_state_subscriber =
          nh.subscribe("current_state", 1,
                       &controllerNode::onCurrentState, this);

      motor_command_publisher =
          nh.advertise<mav_msgs::Actuators>("rotor_speed_cmds", 1);

      control_timer =
          nh.createTimer(ros::Duration(1.0 / hz),
                         &controllerNode::controlLoop, this);
      //
      // ~~~~ end solution

      // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
      //                                 end part 2
      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //  PART 6 [NOTE: save this for last] |  Tune your gains!
      // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
      //
      // Live the life of a control engineer! Tune these parameters for a fast
      // and accurate controller.
      //
      // Controller gains
      //
      //
      // **** FIDDLE WITH THESE! ***
      // Change them in the provided launch file.
      nh.getParam("kx", kx);
      nh.getParam("kv", kv);
      nh.getParam("kr", kr);
      nh.getParam("komega", komega);
      ros::NodeHandle private_nh("~");
      private_nh.param("racing_safety", racing_safety, false);
      private_nh.param("racing_tilt_limit", racing_tilt_limit, racing_tilt_limit);
      private_nh.param("racing_rate_limit", racing_rate_limit, racing_rate_limit);
      ROS_INFO("Gain values:\nkx: %f \nkv: %f \nkr: %f \nkomega: %f\n", kx, kv, kr, komega);

      //
      // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
      //                                 end part 6
      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

      // Initialize constants
      m = 1.0;
      cd = 1e-5;
      cf = 1e-3;
      g = 9.81;
      d = 0.3;
      J << 1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0;
      if (racing_safety) {
        // VNAV2021 Lab4 tesse_quadrotor_controller.Start explicitly sets
        // Unity inertia to (0.1, 0.1, 0.2).  Swapping Unity y/z into ROS gives
        // (0.1, 0.2, 0.1), not the identity used by the Lab3 template.
        J = Eigen::Vector3d(0.1, 0.2, 0.1).asDiagonal();
      }

      // Rotor layout from the LAB3 simulator convention figure:
      // rho1=(+d/sqrt(2), +d/sqrt(2)),
      // rho2=(-d/sqrt(2), +d/sqrt(2)),
      // rho3=(-d/sqrt(2), -d/sqrt(2)),
      // rho4=(+d/sqrt(2), -d/sqrt(2)).
      // Rows are [f_z, tau_x, tau_y, tau_z].
      const double arm = d / std::sqrt(2.0);
      F2W << cf,          cf,          cf,          cf,
             arm * cf,    arm * cf,   -arm * cf,   -arm * cf,
            -arm * cf,    arm * cf,    arm * cf,   -arm * cf,
             cd,         -cd,          cd,         -cd;

      R.setIdentity();
      x.setZero();
      v.setZero();
      omega.setZero();
      xd.setZero();
      vd.setZero();
      ad.setZero();
      yawd = 0.0;
  }

  void onDesiredState(const trajectory_msgs::MultiDOFJointTrajectoryPoint& des_state){

      if (des_state.transforms.empty() || des_state.velocities.empty() ||
          des_state.accelerations.empty()) {
        ROS_WARN_THROTTLE(1.0, "Ignoring malformed desired_state message.");
        return;
      }

      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //  PART 3 | Objective: fill in xd, vd, ad, yawd
      // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
      //
      // 3.1 Get the desired position, velocity and acceleration from the in-
      //     coming ROS message and fill in the class member variables xd, vd
      //     and ad accordingly. You can ignore the angular acceleration.
      //
      // Hint: use "v << vx, vy, vz;" to fill in a vector with Eigen.
      //
      // ~~~~ begin solution
      //
     //     **** FILL IN HERE ***
      xd << des_state.transforms[0].translation.x,
            des_state.transforms[0].translation.y,
            des_state.transforms[0].translation.z;

      vd << des_state.velocities[0].linear.x,
            des_state.velocities[0].linear.y,
            des_state.velocities[0].linear.z;

      ad << des_state.accelerations[0].linear.x,
            des_state.accelerations[0].linear.y,
            des_state.accelerations[0].linear.z;
      //
      // ~~~~ end solution
      //
      // 3.2 Extract the yaw component from the quaternion in the incoming ROS
      //     message and store in the yawd class member variable
      //
      //  Hints:
      //    - use the methods tf2::getYaw(...)
      //    - maybe you want to use also tf2::fromMsg(...)
      //
      // ~~~~ begin solution
      //
     //     **** FILL IN HERE ***
      tf2::Quaternion quat;
      tf2::fromMsg(des_state.transforms[0].rotation, quat);
      yawd = tf2::getYaw(quat);
      has_desired_state = true;
      //
      // ~~~~ end solution
      //
      // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
      //                                 end part 3
      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  }

  void onCurrentState(const nav_msgs::Odometry& cur_state){
      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //  PART 4 | Objective: fill in x, v, R and omega
      // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
      //
      // Get the current position and velocity from the incoming ROS message and
      // fill in the class member variables x, v, R and omega accordingly.
      //
      //  CAVEAT: cur_state.twist.twist.angular is in the world frame, while omega
      //          needs to be in the body frame!
      //
      // ~~~~ begin solution
      //
     //     **** FILL IN HERE ***
      x << cur_state.pose.pose.position.x,
           cur_state.pose.pose.position.y,
           cur_state.pose.pose.position.z;

      v << cur_state.twist.twist.linear.x,
           cur_state.twist.twist.linear.y,
           cur_state.twist.twist.linear.z;

      tf2::Quaternion current_quat;
      tf2::fromMsg(cur_state.pose.pose.orientation, current_quat);
      tf2::Matrix3x3 current_rotation(current_quat);
      for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
          R(row, col) = current_rotation[row][col];
        }
      }

      Eigen::Vector3d omega_world;
      omega_world << cur_state.twist.twist.angular.x,
                     cur_state.twist.twist.angular.y,
                     cur_state.twist.twist.angular.z;
      omega = R.transpose() * omega_world;

      // C (traj_following.launch) starts the controller before D publishes the
      // racing waypoints.  In that interval, hold the vehicle at its measured
      // pose rather than commanding the constructor default of world origin.
      if (!has_current_state && !has_desired_state) {
        xd = x;
        vd.setZero();
        ad.setZero();
        yawd = tf2::getYaw(current_quat);
        ROS_INFO("No desired_state received yet; holding the current pose.");
      }
      has_current_state = true;
      //
      // ~~~~ end solution
      //
      // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
      //                                 end part 4
      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  }

  void controlLoop(const ros::TimerEvent& t){
    // Wait for a real state estimate.  This avoids issuing rotor commands from
    // zero-initialized state vectors during simulator/bridge startup.
    if (!has_current_state) return;

    Eigen::Vector3d ex, ev, er, eomega;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  PART 5 | Objective: Implement the controller!
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    //
    // 5.1 Compute position and velocity errors. Objective: fill in ex, ev.
    //  Hint: [1], eq. (6), (7)
    //
    // ~~~~ begin solution
    //
   //     **** FILL IN HERE ***
    ex = x - xd;
    ev = v - vd;
    //
    // ~~~~ end solution

    // 5.2 Compute the Rd matrix.
    //
    //  Hint: break it down in 3 parts:
    //    - b3d vector = z-body axis of the quadrotor, [1] eq. (12)
    //    - check out [1] fig. (3) for the remaining axes [use cross product]
    //    - assemble the Rd matrix, eigen offers: "MATRIX << col1, col2, col3"
    //
    //  CAVEATS:
    //    - Compare the reference frames in the Lab 3 handout with Fig. 1 in the
    //      paper. The z-axes are flipped, which affects the signs of:
    //         i) the gravity term and
    //        ii) the overall sign (in front of the fraction) in equation (12)
    //            of the paper
    //    - remember to normalize your axes!
    //
    // ~~~~ begin solution
    //
   //     **** FILL IN HERE ***
    Eigen::Vector3d F_des;
    F_des = -kx * ex - kv * ev + m * g * e3 + m * ad;

    if (racing_safety) {
      // The vertical loop does not depend on a changing bank angle and can
      // track height more tightly than the horizontal loop.
      F_des.z() = -10.0 * ex.z() - 6.4 * ev.z() + m*g + m*ad.z();
      // Keep corrections inside the attitude loop's authority.  A large
      // tracking error must not request a near-horizontal or inverted pose.
      F_des.z() = std::max(6.0, std::min(15.0, F_des.z()));
      const double horizontal_limit = F_des.z() * std::tan(racing_tilt_limit);
      const double horizontal_force = F_des.head<2>().norm();
      if (horizontal_force > horizontal_limit)
        F_des.head<2>() *= horizontal_limit / horizontal_force;
    }

    Eigen::Vector3d b3d = F_des.normalized();
    Eigen::Vector3d b1c;
    b1c << std::cos(yawd), std::sin(yawd), 0.0;

    Eigen::Vector3d b2d = b3d.cross(b1c);
    b2d.normalize();

    Eigen::Vector3d b1d = b2d.cross(b3d);
    b1d.normalize();

    Eigen::Matrix3d Rd;
    Rd.col(0) = b1d;
    Rd.col(1) = b2d;
    Rd.col(2) = b3d;
    //
    // ~~~~ end solution
    //
    // 5.3 Compute the orientation error (er) and the rotation-rate error (eomega)
    //  Hints:
    //     - [1] eq. (10) and (11)
    //     - you can use the Vee() static method implemented above
    //
    //  CAVEAT: feel free to ignore the second addend in eq (11), since it
    //          requires numerical differentiation of Rd and it has negligible
    //          effects on the closed-loop dynamics.
    //
    // ~~~~ begin solution
    //
   //     **** FILL IN HERE ***
    Eigen::Matrix3d attitude_error;
    attitude_error =
        0.5 * (Rd.transpose() * R - R.transpose() * Rd);
    er = Vee(attitude_error);
    eomega = omega;
    if (racing_safety) {
      const ros::Time now = ros::Time::now();
      const double dt = (now - previous_control_time).toSec();
      if (!previous_control_time.isZero() && dt > 0 && dt < 0.1) {
        const Eigen::Matrix3d delta_R = previous_Rd.transpose() * Rd;
        Eigen::Vector3d rate = Vee(0.5 * (delta_R - delta_R.transpose())) / dt;
        if (rate.norm() > racing_rate_limit) rate *= racing_rate_limit / rate.norm();
        // Reference updates arrive at 100 Hz; smooth their finite-difference
        // rates instead of differentiating them into impulse torque commands.
        desired_omega += (dt / (0.04 + dt)) * (rate - desired_omega);
      } else if (previous_control_time.isZero() || dt < 0 || dt >= 0.1) {
        // ROS simulated time can issue several control callbacks at the same
        // clock tick.  Preserve the filtered rate when dt==0; resetting it on
        // those callbacks would discard feed-forward almost every frame.
        desired_omega.setZero();
      }
      previous_Rd = Rd;
      previous_control_time = now;
      eomega = omega - R.transpose() * Rd * desired_omega;
    }
    //
    // ~~~~ end solution
    //
    // 5.4 Compute the desired wrench (force + torques) to control the UAV.
    //  Hints:
    //     - [1] eq. (15), (16)

    // CAVEATS:
    //    - Compare the reference frames in the Lab 3 handout with Fig. 1 in the
    //      paper. The z-axes are flipped, which affects the signs of:
    //         i) the gravity term
    //        ii) the overall sign (in front of the bracket) in equation (15)
    //            of the paper
    //
    //    - feel free to ignore all the terms involving \Omega_d and its time
    //      derivative as they are of the second order and have negligible
    //      effects on the closed-loop dynamics.
    //
    // ~~~~ begin solution
    //
   //     **** FILL IN HERE ***
    Eigen::Vector3d M_des;
    M_des = -kr * er - komega * eomega + omega.cross(J * omega);
    if (racing_safety)
      M_des = J * (-kr * er - komega * eomega) + omega.cross(J * omega);
    //
    // ~~~~ end solution

    // 5.5 Recover the rotor speeds from the wrench computed above
    //
    //  Hints:
    //     - [1] eq. (1)
    //
    // CAVEATs:
    //     - we have different conventions for the arodynamic coefficients,
    //       Namely: C_{\tau f} = c_d / c_f
    //               (LHS paper [1], RHS our conventions [lecture notes])
    //
    //     - Compare the reference frames in the Lab 3 handout with Fig. 1 in the
    //       paper. In the paper [1], the x-body axis [b1] is aligned with a
    //       quadrotor arm, whereas for us, it is 45° from it (i.e., "halfway"
    //       between b1 and b2). To resolve this, check out equation 6.9 in the
    //       lecture notes!
    //
    //     - The thrust forces are **in absolute value** proportional to the
    //       square of the propeller speeds. Negative propeller speeds - although
    //       uncommon - should be a possible outcome of the controller when
    //       appropriate. Note that this is the case in unity but not in real
    //       life, where propellers are aerodynamically optimized to spin in one
    //       direction!
    //
    // ~~~~ begin solution
    //
   //     **** FILL IN HERE ***
    Eigen::Vector3d b3 = R * e3;
    double thrust = F_des.dot(b3);

    Eigen::Vector4d wrench;
    wrench << thrust, M_des.x(), M_des.y(), M_des.z();

    Eigen::Vector4d rotor_squared_speed;
    rotor_squared_speed = F2W.inverse() * wrench;
    //
    // ~~~~ end solution
    //
    // 5.6 Populate and publish the control message
    //
    // Hint: do not forget that the propeller speeds are signed (maybe you want
    // to use signed_sqrt function).
    //
    // ~~~~ begin solution
    //
   //     **** FILL IN HERE ***
    mav_msgs::Actuators motor_command;
    motor_command.angular_velocities.resize(4);

    for (int i = 0; i < 4; ++i) {
      motor_command.angular_velocities[i] =
          signed_sqrt(rotor_squared_speed(i));
    }

    motor_command_publisher.publish(motor_command);
    //
    // ~~~~ end solution
    //
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    //           end part 5, congrats! Start tuning your gains (part 6)
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  }
};

int main(int argc, char** argv){
  ros::init(argc, argv, "controller_node");
  controllerNode n;
  ros::spin();
}
