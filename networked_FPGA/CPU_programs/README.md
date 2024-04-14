# CPU Programs

## Network Transmission Formats

* CPU -> FPGA : size_c2f(D)

```
    // Format: for each query
    // packet 0: header (batch_size, ) -> batch size as -1 or 0 means terminate
    //   for the following packets, for each query
    // 		packet 1~k: query_vectors

    // query format: store in 512-bit packets, pad 0 for the last packet if needed
	const int AXI_num_header = 1; 
	const int AXI_num_vec = D % FLOAT_PER_AXI == 0? D / FLOAT_PER_AXI : D / FLOAT_PER_AXI + 1; 
	const int AXI_num_input_per_query = AXI_num_header + AXI_num_vec;
```

* FPGA -> CPU : size_f2c(ef)

```
    // Format: for each query
    // packet 0: header (topK == ef)
    // packet 1~k: topK results, including vec_ID (4-byte) array and dist_array (4-byte)
	//    -> size = ceil(topK * 4 / 64) + ceil(topK * 4 / 64)

    // in 512-bit packets
	const int AXI_num_header = 1; 
    const int AXI_num_results_vec_ID = ef % INT_PER_AXI == 0? ef / INT_PER_AXI : ef / INT_PER_AXI + 1;
    const int AXI_num_results_dist = ef % FLOAT_PER_AXI == 0? ef / FLOAT_PER_AXI : ef / FLOAT_PER_AXI + 1;
	const int AXI_num_output_per_query = AXI_num_header + AXI_num_results_vec_ID + AXI_num_results_dist;
```