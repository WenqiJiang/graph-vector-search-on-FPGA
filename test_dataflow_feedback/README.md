# Test dataflow behavior when there are feedback communications

## Evaluation of dataflow behavior in a single kernel

Suppose we have two PE A and B in a dataflow region connected by HLS stream. A writes a start signal to B (stream A2B); B receives the signal, fetches a value from DRAM channel B, and writes the value back to A (stream B2A); A then writes the value back to memory channel A. However, as simple as this, the generated bitstream will run into a deadlock. 

```
      _____                    ______
     |     |                  |      |
     |     | -> stream A2B -> |      |
MemA-| PE A|                  | PE B |--MemB
     |     | <- stream B2A <- |      |
     |_____|                  |______|
```

Mario's feedback: using Vitis HLS cosim deadlock viewer https://docs.xilinx.com/r/2021.2-English/ug1399-vitis-hls/Cosim-Deadlock-Viewer

Result: once we use this cosim flow, the compilation filed... didn't even started the co-sim at all. If we use csim, the PEs execute the procedure sequentially rather than concurrently, which is wrong. 

So we should use the hw_emu flow instead: 

```
time make all TARGET=hw PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1 > out 2>&1
make run TARGET=hw_emu PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1
```

### Case no for loop 1: without checking FIFO empty

`dataflow_feedback_no_loop_no_empty_check`

Deadlock occurs. Here attaches the error message:

```
// ERROR!!! DEADLOCK DETECTED at 150290000 ns! SIMULATION WILL BE STOPPED! //
//////////////////////////////////////////////////////////////////////////////
/////////////////////////
// Dependence cycle 1:
// (1): Process: vadd_vadd.PE_B_U0
//      Blocked by empty input FIFO 'vadd_vadd.s_A_to_B_U' written by process 'vadd_vadd.PE_A_U0'
// (2): Process: vadd_vadd.PE_A_U0

Info: /OSCI/SystemC: Simulation stopped by user.
//      Blocked by empty input FIFO 'vadd_vadd.s_B_to_A_U' written by process 'vadd_vadd.PE_B_U0'
////////////////////////////////////////////////////////////////////////
// Totally 1 cycles detected!
```

Code:

```
#define EMPTY_CHECK 0

#include <hls_stream.h>

void PE_A(
    // in
    hls::stream<int>& s_B_to_A,
    // out
    hls::stream<int>& s_A_to_B,
    int* out_intersect) {

    // PE A starts the loop
    s_A_to_B.write(1);

    // End by 
#if EMPTY_CHECK
    while (s_B_to_A.empty()) {}
#endif
    int out = s_B_to_A.read();
    out_intersect[0] = out;
}

void PE_B(
    // in
    int* layer_cache,
    hls::stream<int>& s_A_to_B,
    // out
    hls::stream<int>& s_B_to_A) {

#if EMPTY_CHECK
    while (s_A_to_B.empty()) {}
#endif
    int in = s_A_to_B.read();
    int out = layer_cache[in];

    s_B_to_A.write(out);
}
```

### Case no for loop 2: both PEs check empty

`dataflow_feedback_no_loop_with_empty_check`

Passed

### Case no for loop 3: only the first PE checks empty

`dataflow_feedback_no_loop_with_empty_check_only_once_at_feedback_PE`

Passed. (However, in the later version with for loop, this does not work).

In the first PE, the read and write do not have dependencies, thus may lead to compiler bugs.

### Case with for loop 1: both PEs check at each iteration

`dataflow_feedback_loop_with_empty_check_all_iterations`

Passed

### Case with for loop 2: both PEs check only at the first iteration

`dataflow_feedback_loop_with_empty_check_only_once`

Passed

### Case with for loop 3: only the first PE checks empty at the first iteration

`dataflow_feedback_loop_with_empty_check_only_once_at_feedback_PE`

Failed (but deadlock was not detected during simulation). This is weird because the one without loop succeeded.

### Case with multi-PE for loop 1: all PEs check only at the first iteration

`dataflow_feedback_multi_PE_loop_with_empty_check_only_once`

Succeed. 

### Case with multi-PE for loop 1: all PEs check only at the first iteration (only the first two PEs check)

`dataflow_feedback_multi_PE_loop_with_empty_check_only_once_subset_PEs`

Failed.

Conclusion: for multiple PEs, each PE has to check at the first iteration. 