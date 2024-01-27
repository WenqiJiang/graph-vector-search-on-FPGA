#pragma once

#include "types.hpp"

void fetch_neighbor_ids(
	// in initialization
	const int query_num,
	const int max_link_num_top, 
	const int max_link_num_base,
	// in runtime (should from DRAM)
	const float* links_top,
	const float* links_base,
	// in runtime (stream)
	hls::stream<int>& s_finish_query_task_scheduler,

	// out (stream)
	hls::stream<int>& s_fetched_neighbor_ids,
	hls::stream<int>& s_finish_query_fetch_neighbor_ids
) {
	
}

void fetch_vectors(
	// in initialization
	const int query_num,
	const int d,
	// in runtime (should from DRAM)
	const float* vectors_top,
	const float* vectors_base,
	// in runtime (stream)
	hls::stream<int>& s_fetched_neighbor_ids_replicated,
	hls::stream<int>& s_finish_query_replicate_s_fetched_neighbor_ids,
	
	// out (stream)
	hls::stream<float>& s_fetched_vectors,
	hls::stream<int>& s_finish_query_fetch_vectors
) {
	
}

// void axi_to_float(
//     // in (initialization)
//     const int query_num,
//     const int vec_AXI_num,
//     // in runtime (stream)
//     hls::stream<ap_uint<512>>& s_fetched_vectors_filtered,
//     hls::stream<int>& s_finish_query_check_visited,
    
//     // out (stream)
//     hls::stream<float> (&s_float_vectors)[FLOAT_PER_AXI],
//     hls::stream<int>& s_finish_query_internal
// ) {
//     for (int qid = 0; qid < query_num; qid++) {
//         for (int i = 0; i < vec_AXI_num; i++) {
//             ap_uint<512> vec_AXI = s_fetched_vectors_filtered.read();
//             for (int j = 0; j < FLOAT_PER_AXI; j++) {
//                 ap_uint<32> vec_uint = vec_AXI.range(32 * (j + 1) - 1, 32 * j);
//                 float vec_float = *((float*) (&vec_uint));
//                 s_float_vectors[j].write(vec_float);
//             }
//         }
//     }
// )

void check_visited(
	// in (initialization)
	const int query_num,
	const int d,
	// in runtime (stream)
	hls::stream<int>& s_fetched_neighbor_ids_replicated,
	hls::stream<float>& s_fetched_vectors,
	hls::stream<int>& s_finish_query_fetch_vectors,

	// out (stream)
	hls::stream<float>& s_fetched_vectors_filtered,
	hls::stream<int>& s_finish_query_check_visited
) {
	
}

void results_collection(
	// in (initialization)
	const int query_num,
	// in runtime (stream)
	hls::stream<float>& s_distances_replicated,
	hls::stream<int>& s_finish_query_replicate_distances,

	// out (DRAM)
	float* out_id,
	float* out_dist
) {

}