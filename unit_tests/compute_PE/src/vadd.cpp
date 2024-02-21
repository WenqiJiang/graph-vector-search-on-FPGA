#include "compute.hpp"
#include "constants.hpp"
#include "types.hpp"

#define D_MAX 1024

void read_memory(
	const int query_num, 
	const int num_fetched_vectors_per_query,
	const int d,

    // in runtime (from DRAM)
	const ap_uint<512>* query_vectors,
   	const ap_uint<512>* fetched_vectors,

	// out streams
    hls::stream<int>& s_finish_query_in, // finish the current query
	hls::stream<ap_uint<512>>& s_query_vectors,
    hls::stream<ap_uint<512>>& s_fetched_vectors,
	hls::stream<cand_t>& s_fetched_neighbor_ids_replicated,
    hls::stream<bool>& s_fetch_valid // if visited, not valid
) {

	ap_uint<512> query_buffer[D_MAX / FLOAT_PER_AXI];
	ap_uint<512> fetched_buffer[D_MAX / FLOAT_PER_AXI];
	cand_t neighbor_ids_dummy = {0, 0};
	bool fetch_valid = true;

	for (int j = 0; j < d / FLOAT_PER_AXI; j++) {
		#pragma HLS pipeline II=1
		query_buffer[j] = query_vectors[j];
		fetched_buffer[j] = fetched_vectors[j];
	}

	for (int i = 0; i < query_num; i++) {
		// write query
		for (int j = 0; j < d / FLOAT_PER_AXI; j++) {
			#pragma HLS pipeline II=1
			s_query_vectors.write(query_buffer[j]);
		}

		// write fetched vector & s_fetched_neighbor_ids_replicated & s_fetch_valid
		for (int k = 0; k < num_fetched_vectors_per_query; k++) {
			for (int j = 0; j < d / FLOAT_PER_AXI; j++) {
				#pragma HLS pipeline II=1
				s_fetched_vectors.write(fetched_buffer[j]);
			}
			s_fetched_neighbor_ids_replicated.write(neighbor_ids_dummy);
			s_fetch_valid.write(fetch_valid);
		}

		// write finish query
		s_finish_query_in.write(1);
	}
}

void write_memory(
	const int query_num, 
	const int num_fetched_vectors_per_query,
	hls::stream<result_t>& s_distances,
	hls::stream<int>& s_finish_query_out, // finish the current query
	float* out_dist
) {
	for (int i = 0; i < query_num - 1; i++) {
		// read distance
		for (int j = 0; j < num_fetched_vectors_per_query; j++) {
			#pragma HLS pipeline II=1
			volatile result_t dist = s_distances.read();
		}

		// read finish query
		volatile int finish = s_finish_query_out.read();
	}
	for (int j = 0; j < num_fetched_vectors_per_query; j++) {
		#pragma HLS pipeline II=1
		result_t dist = s_distances.read();
		out_dist[j] = dist.dist;
	}
	volatile int finish = s_finish_query_out.read();
}



extern "C" {

void vadd(  
	// in initialization
	const int query_num, 
	const int num_fetched_vectors_per_query,
	const int d,

    // in runtime (from DRAM)
	const ap_uint<512>* query_vectors,
   	const ap_uint<512>* fetched_vectors,

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

#pragma HLS dataflow

    hls::stream<int> s_finish_query_in; // finish the current query
#pragma HLS stream variable=s_finish_query_in depth=16

	hls::stream<ap_uint<512>> s_query_vectors;
#pragma HLS stream variable=s_query_vectors depth=128

    hls::stream<ap_uint<512>> s_fetched_vectors; 
#pragma HLS stream variable=s_fetched_vectors depth=512

	hls::stream<cand_t> s_fetched_neighbor_ids_replicated;
#pragma HLS stream variable=s_fetched_neighbor_ids_replicated depth=512

    hls::stream<bool> s_fetch_valid; // if visited, not valid
#pragma HLS stream variable=s_fetch_valid depth=512


	read_memory(
		query_num, 
		num_fetched_vectors_per_query,
		d,
		query_vectors,
		fetched_vectors,
		s_finish_query_in,
		s_query_vectors,
		s_fetched_vectors,
		s_fetched_neighbor_ids_replicated,
		s_fetch_valid
	);

    hls::stream<result_t> s_distances; 
#pragma HLS stream variable=s_distances depth=512

    hls::stream<int> s_finish_query_out; // finish the current query
#pragma HLS stream variable=s_finish_query_out depth=16

	compute_distances(
		// in initialization
		query_num,
		d,
		// in runtime (stream)
		s_query_vectors,
		s_fetched_vectors,
		s_fetch_valid,
		s_fetched_neighbor_ids_replicated,
		s_finish_query_in,
		
		// out (stream)
		s_distances,
		s_finish_query_out
	);

	write_memory(
		query_num, 
		num_fetched_vectors_per_query,
		s_distances,
		s_finish_query_out,
		out_dist
	);

}

}
