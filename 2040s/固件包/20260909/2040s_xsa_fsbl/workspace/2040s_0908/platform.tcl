# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/startest4070/workspace/2040s_0908/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/startest4070/workspace/2040s_0908/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {2040s_0908}\
-hw {/home/startest4070/share/2040s_prj/0908xsa/2040s_0908.xsa}\
-no-boot-bsp -out {/home/startest4070/workspace}

platform write
domain create -name {standalone_ps7_cortexa9_0} -display-name {standalone_ps7_cortexa9_0} -os {standalone} -proc {ps7_cortexa9_0} -runtime {cpp} -arch {32-bit} -support-app {zynq_fsbl}
platform generate -domains 
platform write
platform generate -quick
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
