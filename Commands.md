
### Build

Make hardware: 

```
time make all TARGET=hw PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1 > out_hw 2>&1
```

Make host:

```
make host
// g++ -g -std=c++11 -I/home/wejiang/opt/xilinx/xrt/include -o host src/host.cpp -L/home/wejiang/opt/xilinx/xrt/lib -lxilinxopencl -pthread -lrt
```

### Emulation

Make emu:

```
time make all TARGET=hw_emu PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1 > out_hw_emu 2>&1
```

Run emu:

```
make run TARGET=hw_emu PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1  > out_run_hw_emu 2>&1
```

Kill all emulations:

```
ps aux | grep -i hw_emu | grep wejiang | awk '{print $2}' | xargs -i kill -9 {}  
```

### Kill

Kill all Xilinx / Vitis process (prevent failure in removing files):

```
ps aux | grep -i xilinx | grep wejiang | awk '{print $2}' | xargs -i kill -9 {}  
```

Reset FPGA state:

```
xbutil reset --device
```

Validate FPGA:

```
xbutil validate --device
```