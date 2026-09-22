#!/bin/bash
set -e
source /opt/ros/noetic/setup.bash
source /root/vnav_ws/devel/setup.bash
cd '/mnt/d/02_Academic/大三/机器人小组项目/LAB2/tools'
g++ -std=c++14 -fPIC mesh_probe.cpp -o /tmp/lab2_mesh_probe $(pkg-config --cflags --libs rviz)
/tmp/lab2_mesh_probe "$PWD/mesh_probe_hardware.png"
LIBGL_ALWAYS_SOFTWARE=1 /tmp/lab2_mesh_probe "$PWD/mesh_probe_software.png"
