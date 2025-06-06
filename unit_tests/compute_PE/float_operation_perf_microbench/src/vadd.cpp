#include "compute.hpp"
#include "constants.hpp"
#include "types.hpp"

#define D_MAX 1024

void read_memory(
	const int query_num, 
	const int num_fetched_vectors_per_query,
	const int d,
	const bool valid,

    // in runtime (from DRAM)
	const ap_uint<512>* query_vectors,
   	const ap_uint<512>* fetched_vectors,
    hls::stream<int>& s_finish_query_write_memory, // finish the current query

	// out streams
    hls::stream<int>& s_finish_query_read_memory, // finish the current query
	hls::stream<ap_uint<512>>& s_query_vectors,
    hls::stream<ap_uint<512>>& s_fetched_vectors,
	hls::stream<cand_t>& s_fetched_neighbor_ids_replicated,
    hls::stream<bool>& s_fetch_valid // if visited, not valid
) {

	ap_uint<512> query_buffer[D_MAX / FLOAT_PER_AXI];
	ap_uint<512> fetched_buffer[D_MAX / FLOAT_PER_AXI];
	cand_t neighbor_ids_dummy = {0, 0};
	bool fetch_valid = valid;

	const int vec_AXI_num = d % FLOAT_PER_AXI == 0? d / FLOAT_PER_AXI : d / FLOAT_PER_AXI + 1; 

	for (int j = 0; j < vec_AXI_num; j++) {
		#pragma HLS pipeline II=1
		query_buffer[j] = query_vectors[j];
		fetched_buffer[j] = fetched_vectors[j];
	}

	for (int i = 0; i < query_num; i++) {
		// write query
		for (int j = 0; j < vec_AXI_num; j++) {
			#pragma HLS pipeline II=1
			s_query_vectors.write(query_buffer[j]);
		}

		// write fetched vector & s_fetched_neighbor_ids_replicated & s_fetch_valid
		for (int k = 0; k < num_fetched_vectors_per_query; k++) {
			for (int j = 0; j < vec_AXI_num; j++) {
				#pragma HLS pipeline II=1
				s_fetched_vectors.write(fetched_buffer[j]);
			}
			s_fetched_neighbor_ids_replicated.write(neighbor_ids_dummy);
			s_fetch_valid.write(fetch_valid);
		}

		// write finish query
		s_finish_query_read_memory.write(1);

		// wait for the next query
		while (s_finish_query_write_memory.empty()) {}
		volatile int finish = s_finish_query_write_memory.read();
	}
}

void write_memory(
	const int query_num, 
	const int num_fetched_vectors_per_query,
	hls::stream<result_t>& s_distances,
	hls::stream<int>& s_finish_query_compute, // finish the current query
	float* out_dist,
	hls::stream<int>& s_finish_query_write_memory
) {
	result_t dist;
	volatile int finish;
	for (int i = 0; i < query_num - 1; i++) {
		// read distance
		for (int j = 0; j < num_fetched_vectors_per_query; j++) {
			#pragma HLS pipeline II=1
			dist = s_distances.read();
		}

		// read finish query
		finish = s_finish_query_compute.read();
		s_finish_query_write_memory.write(1);
	}
	out_dist[0] = dist.dist;
	for (int j = 0; j < num_fetched_vectors_per_query; j++) {
		#pragma HLS pipeline II=1
		dist = s_distances.read();
		out_dist[j] = dist.dist;
	}
	finish = s_finish_query_compute.read();
	s_finish_query_write_memory.write(1);
}



extern "C" {

void vadd(  
	// in initialization
	const int query_num, 
	const int num_fetched_vectors_per_query,
	const int d,
	const bool valid,

    // in runtime (from DRAM)
	const float* query_vectors,
   	const float* fetched_vectors,

    // out
	float* out_dist
    )
{
// Share the same AXI interface with several control signals (but they are not allowed in same dataflow)
//    https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Controlling-AXI4-Burst-Behavior

// in runtime (from DRAM)
#pragma HLS INTERFACE m_axi port=query_vectors offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=fetched_vectors offset=slave bundle=gmem1 // share the same AXI interface with query_vectors

// out
#pragma HLS INTERFACE m_axi port=out_dist  offset=slave bundle=gmem10

	float local_array_A[D_MAX];
	float local_array_B[D_MAX];
	float local_array_C[D_MAX];

	for (int i = 0; i < query_num; i++) {
		#pragma HLS pipeline II=1
		local_array_A[i] = query_vectors[i];
		local_array_B[i] = fetched_vectors[i];
	}


	// 140MHz: Pipelining result : Target II = 1, Final II = 1, Depth = 5, loop 'ADD_LOOP'
	// 200MHz: Pipelining result : Target II = 1, Final II = 1, Depth = 8, loop 'ADD_LOOP'
	ADD_LOOP:	
	for (int i = 0; i < query_num; i++) {
#pragma HLS pipeline II=1
// #pragma HLS BIND_OP variable=local_array_C op=fmul latency=1
		local_array_C[i] = local_array_A[i] + local_array_B[i];	
	}

	// 140MHz: Pipelining result : Target II = 1, Final II = 1, Depth = 5, loop 'MULT_LOOP'
	// 200MHz: Pipelining result : Target II = 1, Final II = 1, Depth = 5, loop 'MULT_LOOP'
	MULT_LOOP:
	for (int i = 0; i < query_num; i++) {
#pragma HLS pipeline II=1
// #pragma HLS BIND_OP variable=local_array_C op=fmul latency=1
		local_array_C[i] = local_array_A[i] * local_array_B[i];
	}

	for (int i = 0; i < query_num; i++) {
		#pragma HLS pipeline II=1
		out_dist[i] = local_array_C[i];
	}
}

}
