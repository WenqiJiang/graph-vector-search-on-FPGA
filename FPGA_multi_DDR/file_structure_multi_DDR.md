# Inter-query Parallel + Multi DRAM Channel

PEs working on different queries.

## V1 : FPGA_inter_query_v1

Based on single channel V1.2 FPGA_single_DDR_single_layer_v1.2_support_multi_batch

We support breaking down all queries to batches, so we prepare for measuring performance with smaller batches, as well as preparing for the networked version.

# Intra-query Parallel + Multi DRAM Channel

Multiple PEs working on the same query.


## V1 : FPGA_intra_query_v1_async_bloom

Based on single-channel V1 : FPGA_single_DDR_single_layer_v1_async

## V1.1 : FPGA_intra_query_v1.1_async_opt_mem : update memory access + reduce bloom filter number to 3

Based on single-channel V1.1 : FPGA_single_DDR_single_layer_v1.1_async_opt_mem

Instead of manual burst with variable length, here we use fixed length (D), and also reduce bloom filter number to 3. This is because the burst memory access of this version is very fast (close to 40 ns per D=128 vector given 200 MHz, i.e., 8 cycles), we would like reduce blooom II to (3 * 2 = 6 cycles), otherwise it can become the bottleneck.