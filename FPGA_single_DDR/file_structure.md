# Single DRAM Channel Implementation

## V1: FPGA_single_DDR_v1_no_bloom_filter

Store visited tag in memory. Each vector read requires 1 read and then 1 optional write, that not only leading to high latency but also prevent prefetching opportunities due to the dependencies. 

## V2: Use bloom filter 

To remove unnecessary reads and allows burst read.

## V2.1: allow multiple top candidates per batch

Upon v2, allowing multiple top candidates to be popped and evaluated per batch.

## V2.2: allow asynchronous batching + multiple top candidates per batch

Upon v2.1, further allowing multiple batches in the pipeline. The user can specify the desired maximum stage numbers and maximum candidate batch size per stage.

## V2.3: update coding style & debug signals & optimize result write

Minor updates based on V2.2.

Performance wise: 
* use two loops to write result distances and IDs, so we make sure burst is inferred.
* add entry node ID as part of the evaluation in the first iteration, so avoid redundant visits

Debug wise: add signal to track the total amount of upper / base layer nodes we visited. 

Coding style wise: use the wrapped bloom-fetch-compute unit.