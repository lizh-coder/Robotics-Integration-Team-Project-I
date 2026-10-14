# VNAV Lab 4 — Drone Control and Racing

This directory is the complete Lab 4 submission folder required by the VNAV handout.  It contains the individual written work and Team 20's ROS source code for the simulated drone-racing task.

## Contents

| Directory or file | Contents |
| --- | --- |
| `01_Individual_Deliverables/` | Typeset PDF answers for Deliverables 1 and 2, plus the English report. |
| `02_Team_Racing_Source_Code/` | ROS packages, Lab 4 dependencies, and RViz configuration used for Deliverable 3. |
| `03_Runtime_Evidence/` | Machine-readable record from the verified fastest full-course run. |
| `rviz_drone_race.txt` | Required public video and rosbag links for the final course submission. |

## Team racing implementation

The racing workspace contains the following packages.

| Package | Role |
| --- | --- |
| `planner_pkg` | Reads gate poses and publishes race waypoints.  `simple_traj_planner.cpp` supports the Part 0 first-gate hover test. |
| `trajectory_generation_pkg` | Builds a yaw-continuous, minimum-snap trajectory through all gates, enforces velocity/acceleration limits, and publishes the desired state. |
| `controller_pkg` | Geometric controller with the stabilized racing gains and rate/tilt limits used to track the trajectory. |
| `dependencies/` | The source dependencies supplied with the Lab 4 codebase, retained so the workspace can be built reproducibly. |

The verified run crossed all 12 gate planes in order with no collision.  Its measured finish-plane time was **20.818 s** from the race start; `03_Runtime_Evidence/twenty_verified.json` records the gate crossings and tracking metrics.

## Build and run

Use ROS Noetic and an existing VNAV workspace that already contains `tesse_ros_bridge` from the previous lab.  Copy the contents of `02_Team_Racing_Source_Code/` into the workspace `src/` directory, build, and source the workspace.

```bash
cd ~/vnav_ws/src
cp -r /path/to/Robotics-Integration-Team-Project-I/lab4/02_Team_Racing_Source_Code/. .
cd ~/vnav_ws
catkin build
source devel/setup.bash
```

Start the Lab 4 Unity simulator, then launch these ROS nodes in this order:

```bash
roslaunch tesse_ros_bridge tesse_quadrotor_bridge.launch
roslaunch trajectory_generation traj_following.launch
roslaunch planner_pkg traj_gen.launch
```

The last command begins waypoint publication and starts the race.  The simulator binary itself is not included because it is distributed separately in the official VNAV Lab 4 release.

## Before the final course submission

The individual PDFs and report identify Li Zehao (24020036039).  Add publicly viewable Google Drive or Dropbox links for the race video and the rosbag (`/current_state` and `/desired_state`) in `rviz_drone_race.txt`, as required by the handout.
