# Performance fetch vectors

Read write performance can be lookedup here in the spatial join project: https://github.com/WenqiJiang/spatial-join-on-FPGA/tree/main/test_multi_kernel#compare-random-read-performance 

## read vector + visited tag + write tag

### V1

1 read + 1 write per query; no prefetching / batched pipeline fetching, because the next read operation may be influenced by the current write operation, especially in the scenario where we use multiple nodes and they have overlap on neighbors

```
	const int AXI_num_per_vector = d % FLOAT_PER_AXI == 0? 
		d / FLOAT_PER_AXI + 1 : d / FLOAT_PER_AXI + 2; // 16 for d = 512 + visited padding

	for (int i = 0; i < AXI_num_per_vector; i++) {
	#pragma HLS pipeline II=1
		ap_uint<512> vector_AXI = db_vectors[start_addr + i];
		if (i < AXI_num_per_vector - 1) {
			s_fetched_vectors.write(vector_AXI);
		} else {
			ap_uint<32> last_visit_qid = vector_AXI.range(31, 0);
			valid = level_id > 0? true : last_visit_qid != qid;
			s_fetch_valid.write(valid);
		}
	}
```

Performance:

```
    int query_num = 1000;
	int read_iter_per_query = 1000;
	int d = 128;
```

780.183 ms @200 MHz

1 group (read + write) = 780.183 ms / 1e3 / 1e3 = 780 ns. Very high -> almost 10x to the best pipelined read (throughput translated to 83 ns / len=8 vector).

Conclusion: the way to go is to use bloom filter -> which can lead to up to 10x performance improvement, and also ~2x number of visits given the duplicates. 


## Only read vectors

### V1: no prefetching, no batching

1 read per query; no prefetching / batched pipeline fetching

```
    int query_num = 1000;
	int read_iter_per_query = 1000;
	int d = 128;
```

433.682 ms @200 MHz -> 433 ns per read


### V1.1: no prefetching, w/ batching

Compared to V1, add a batch size indicating the number of reads, but still no prefetching. 

Literally no improvement over V1.


### V1.2: prefetching + batching

set max_burst_length = 16. I have tried max_burst_length of 32 and 64, and the performance is exactly the same.

Prefetching + batching.

Large read batches (1000): 

```
    int query_num = 1000;
	int read_iter_per_query = 1000;
	int d = 128;
```

125.515 ms @200 MHz -> 125 ns per read (length of 8) == 25 cycles (given lenght = 8, the pure latency is 25 - 8 + 1 = 18 cycles)

Medium read batches (100): 

13.0161 ms @200 MHz -> 130 ns per read (length of 8) 

Small read batches (10):

1.8214 @200 MHz -> 182 ns per read (length of 8) -> 1.46x time compared to maximal batch sizes. 