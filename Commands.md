
Make hardware: 

```
time make all TARGET=hw PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1 > out 2>&1
```

Kill all Xilinx / Vitis process (prevent failure in removing files):

```
ps aux | grep -i xilinx | grep wejiang | awk '{print $2}' | xargs -i kill -9 {}  
```

### Emulation

Make emu:

```
time make all TARGET=hw_emu PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1 > out 2>&1
```

Run emu:

```
make run TARGET=hw_emu PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1
```

Kill all emulations:

```
ps aux | grep -i hw_emu | grep wejiang | awk '{print $2}' | xargs -i kill -9 {}  
```