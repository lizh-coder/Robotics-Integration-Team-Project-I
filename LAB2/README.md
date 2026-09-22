# LAB2 - ROS 安装与使用

本目录按 MIT 16.485 VNAV Lab 2 的要求整理。实验代码复用了项目中已有的
`LAB1/vnav-codes/lab2/two_drones_pkg` 官方骨架，并复制到 `LAB2/lab2/` 后完成了
两个节点中的待实现部分。

## 环境

实验使用 ROS 1（课程网页的示例环境为 Ubuntu + ROS Noetic）、catkin、tf2 和 rviz。
请在每个终端先加载 ROS 和本实验工作区：

```bash
source /opt/ros/noetic/setup.bash
source ~/vnav_ws/devel/setup.bash
```

如果还没有工作区，可以执行：

```bash
mkdir -p ~/vnav_ws/src
cd ~/vnav_ws
catkin init
cp -a "/mnt/d/02_Academic/大三/机器人小组项目/LAB2/lab2/two_drones_pkg" src/
catkin build
source devel/setup.bash
```

如果已有同名包，直接复用并重新 `catkin build` 即可，不需要重新下载整个 VNAV 仓库。

## 运行

静态场景：

```bash
roslaunch two_drones_pkg two_drones.launch static:=true
```

动态场景（交付物 2、3）：

```bash
roslaunch two_drones_pkg two_drones.launch
```

在 rviz 中将 Fixed Frame 从 `world` 改为 `av1`，可以观察 AV2 相对于 AV1 的闭合曲线。
也可以用下面的命令检查结果：

```bash
rosnode list
rostopic list
rostopic echo /visuals
rosrun tf tf_echo av1 av2
```

## 提交内容

- math_derivations.tex：交付物 4--6 的 LaTeX 数学排版源文件；
- `lab2/two_drones_pkg/`：完整 ROS package；
- `实验报告.md`：交付物 1--5 的回答，以及可选交付物 6；
- `lab2/two_drones_pkg/src/frames_publisher_node.cpp`：发布动态 tf；
- `lab2/two_drones_pkg/src/plots_publisher_node.cpp`：查询相对 tf 并绘制轨迹。
