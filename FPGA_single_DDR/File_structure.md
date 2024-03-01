# Single DRAM Channel Implementation

## V1: FPGA_single_DDR_v1_no_bloom_filter

Store visited tag in memory. Each vector read requires 1 read and then 1 optional write, that not only leading to high latency but also prevent prefetching opportunities due to the dependencies. 