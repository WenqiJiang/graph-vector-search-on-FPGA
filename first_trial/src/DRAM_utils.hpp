#pragma once

#include "types.hpp"

void fetch_neighbor_ids(
	// in initialization
	const int query_num,
	const int max_link_num_top, 
	const int max_link_num_base,
	// in runtime (should from DRAM)
	const ap_uint<512>* links_top,
	const ap_uint<512>* links_base,
	// in runtime (stream)
	hls::stream<cand_t>& s_top_candidates,
	hls::stream<int>& s_finish_query_task_scheduler,

	// out (stream)
	hls::stream<int>& s_num_neighbors,
	hls::stream<cand_t>& s_fetched_neighbor_ids,
	hls::stream<int>& s_finish_query_fetch_neighbor_ids
) {

	bool first_iter_s_top_candidates = true;
	// bool first_iter_s_finish_query_task_scheduler = true;
	
	for (int qid = 0; qid < query_num; qid++) {

		while (true) {

			// check query finish
			if (!s_finish_query_task_scheduler.empty() && s_fetched_neighbor_ids.empty()) {
				s_finish_query_fetch_neighbor_ids.write(s_finish_query_task_scheduler.read());
				break;
			} else {
				// receive task
				if (first_iter_s_top_candidates) {
					while (s_top_candidates.empty()) {}
					first_iter_s_top_candidates = false;
				}
				cand_t reg_cand = s_top_candidates.read();
				int currObj = reg_cand.nodeID;
				int level = reg_cand.level;

				// read links
				int start_addr;
				// TODO: compute the correct address, and count number & read links

			}
		}
	
	}
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
	
	bool first_iter_s_fetched_neighbor_ids_replicated = true;

	for (int qid = 0; qid < query_num; qid++) {
		while (true) {
			// check query finish
			if (!s_finish_query_replicate_s_fetched_neighbor_ids.empty() && s_fetched_vectors.empty()) {
				s_finish_query_fetch_vectors.write(s_finish_query_replicate_s_fetched_neighbor_ids.read());
				break;
			} else {
				// receive task
				if (first_iter_s_fetched_neighbor_ids_replicated) {
					while (s_fetched_neighbor_ids_replicated.empty()) {}
					first_iter_s_fetched_neighbor_ids_replicated = false;
				}
				cand_t reg_cand = s_fetched_neighbor_ids_replicated.read();
				int node_id = reg_cand.nodeID;
				int level = reg_cand.level;

				// read vectors
				int start_addr;
				// TODO: add proper start addr and write out distance
			}
		}
	}
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