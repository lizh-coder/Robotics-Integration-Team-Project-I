# Original course mesh: rendering diagnosis

The original `quadrotor.dae` was restored without modification. SHA256:
`5F23AB599E4726A00BF60D0F6C6577BECABDD948E2307B06C6A076D8579D8A7D`.

The earlier DAE/OBJ/STL substitutions did not establish the cause. The STL
conversion additionally produced a normal-buffer loading error. Those generated
files are no longer referenced by the publisher.

`mesh_probe.cpp` uses the installed RViz `loadMeshFromResource` and OGRE renderer
to load the original package URI and render the same scene on two GL backends.
Both runs load 522 submeshes, 35112 vertices and 35112 indices, with bounds
(-0.268418, -0.266089, -0.0305283) to (0.263295, 0.265345, 0.0405656).

- D3D12 (NVIDIA GeForce RTX 4050 Laptop GPU): image is blank.
- llvmpipe (LLVM 12.0.0, 256 bits): original quadrotor is visible.

Evidence: `mesh_probe_hardware.png` and `mesh_probe_software.png`.
This local comparison isolates the backend-dependent rendering failure; it does
not identify the exact internal driver bug or establish a general WSLg limitation.

Fix: `two_drones.launch` defaults `software_rendering:=true` and passes
`LIBGL_ALWAYS_SOFTWARE` only to its RViz node. The publisher again uses the
original DAE mesh markers and course colors. TF and trajectory logic are unchanged.
Software rendering can use more CPU. Other hosts can opt out with
`software_rendering:=false`.

Run dynamic mode:
```bash
source /opt/ros/noetic/setup.bash
source /root/vnav_ws/devel/setup.bash
roslaunch two_drones_pkg two_drones.launch
```
