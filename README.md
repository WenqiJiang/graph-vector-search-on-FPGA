# FPGA Accelerator For Graph-based Vector Search

**The official repository has been moved to: https://github.com/fpgasystems/Falcon-accelerate-graph-vector-search**

This is a repository for our VLDB'25 paper [Fast Graph Vector Search via Hardware Acceleration and Delayed-Synchronization Traversal](https://arxiv.org/abs/2406.12385)

```
@article{jiang2024accelerating,
  title={Accelerating Graph-based Vector Search via Delayed-Synchronization Traversal},
  author={Jiang, Wenqi and Hu, Hang and Hoefler, Torsten and Alonso, Gustavo},
  journal={arXiv preprint arXiv:2406.12385},
  year={2024}
}
```

Software baseline: https://github.com/HangHu-sys/vector_search_baselines

## All Bitstreams & Experiments

### Local Bitstreams & Experiments

#### Bitstreams

```
├── FPGA_multi_DDR
│   ├── FPGA_inter_query_v1.3_longer_FIFO_alt_PR
│   ├── FPGA_intra_query_v1.5_support_batching_longer_FIFO
```

Each bitstream may have different FIFO lengths that can pass P&R (already written in constant.hpp); Also double-check the connectivity.cfg file in case for different dimensionality, we need different P&R directives. 

3 bitstreams for inter-query:

`FPGA_inter_query_v1.3_longer_FIFO_alt_PR`, with D=96, 128, 384 for Deep, SIFT, and SBERT; 4 channels

3 x 3 = 9 bitstreams for inter-query:

`FPGA_intra_query_v1.5_support_batching_longer_FIFO` , with D=96, 128, 384 for Deep, SIFT, and SBERT; and 1, 2, 4 channels for scalability tests

#### Experiments

See `perf_test_scripts/README.md` for the evaluation of throughput, latency, and recall for various settings. 

### Networked bitstreams

#### Bitstreams

```
├── networked_FPGA
│   ├── kernel
│   │   └── user_krnl
│   │       ├── FPGA_inter_query_v1_3
│   │       ├── FPGA_intra_query_v1_5
```

Each bitstream may have different FIFO lengths that can pass P&R (already written in constant.hpp); Also double-check the connectivity.cfg file in case for different dimensionality, we need different P&R directives. 

For networked version, SBERT dataset would not work due to P&R problems with D=384.

2 bitstreams for inter-query:

`FPGA_inter_query_v1_3`, with D=96, 128; 4 channels

2 bitstreams for intra-query:

`FPGA_intra_query_v1_5`, with D=96, 128; 4 channels

#### Experiments

See `networked_FPGA/CPU_programs/README.md` for the evaluation of latency for various settings. 

## Folder Organization

Here shows the most useful folders

```
├── FPGA_multi_DDR
│   ├── FPGA_inter_query_v1.3_longer_FIFO_alt_PR
│   ├── FPGA_intra_query_v1.5_support_batching_longer_FIFO
├── FPGA_single_DDR
├── networked_FPGA
│   ├── CPU_programs
│   ├── kernel
│   │   └── user_krnl
│   │       ├── FPGA_inter_query_v1_3
│   │       ├── FPGA_intra_query_v1_5
│   ├── host
│   │   ├── FPGA_inter_query_v1_3
│   │   ├── FPGA_intra_query_v1_5
├── perf_test_scripts
│   └── saved_df
├── plots
│   ├── images
├── test_dataflow_feedback
└── unit_tests
    ├── bloom_and_hash
    ├── bloom_fetch_compute
    ├── compute_PE
    ├── fetch_vectors
    └── priority_queue
```

`FPGA_multi_DDR`: multi-DDR version FPGA implementations

`networked_FPGA`: networked version of multi-DDR version FPGA implementations, with exactly the same kernel implemented in `FPGA_multi_DDR`

`perf_test_scripts`: all the plots used to measure the performance of local FPGAs

`plots`: all the plotting scripts

(unused) `FPGA_single_DDR`: single-DDR version FPGA implementations. Used only for developments.

(unused) `test_dataflow_feedback`: test the dataflow feedback loop behavior. Used only for developments.

(unused) `unit_tests`: benchmark the performance of different building blocks. Used only for developments.
