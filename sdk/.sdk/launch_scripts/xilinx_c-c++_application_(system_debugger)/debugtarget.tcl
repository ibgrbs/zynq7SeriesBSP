connect -url tcp:127.0.0.1:3121
source D:/GITHUB/zynqBSP/zynq7SeriesBSP/sdk/ZyboBase/ps7_init.tcl
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo Z7 210351B3FB7BA"} -index 0
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent Zybo Z7 210351B3FB7BA" && level==0} -index 1
fpga -file D:/GITHUB/zynqBSP/zynq7SeriesBSP/sdk/ZyboBase/BaseDesign_wrapper.bit
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo Z7 210351B3FB7BA"} -index 0
loadhw -hw D:/GITHUB/zynqBSP/zynq7SeriesBSP/sdk/ZyboBase/system.hdf -mem-ranges [list {0x40000000 0xbfffffff}]
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo Z7 210351B3FB7BA"} -index 0
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "ARM*#0" && jtag_cable_name =~ "Digilent Zybo Z7 210351B3FB7BA"} -index 0
dow D:/GITHUB/zynqBSP/zynq7SeriesBSP/sdk/Uart_Driver/Debug/Uart_Driver.elf
configparams force-mem-access 0
bpadd -addr &main
