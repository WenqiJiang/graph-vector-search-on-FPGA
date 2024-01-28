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
	hls::stream<int>& s_finish_query_in,

	// out (stream)
	hls::stream<int>& s_num_neighbors,
	hls::stream<cand_t>& s_fetched_neighbor_ids,
	hls::stream<int>& s_finish_query_out
) {

	for (int qid = 0; qid < query_num; qid++) {

		while (true) {

			// check query finish
			if (!s_finish_query_in.empty() && s_top_candidates.empty()) {
				s_finish_query_out.write(s_finish_query_in.read());
				break;
			} else if (!s_top_candidates.empty()) {
				// receive task
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
	hls::stream<int>& s_finish_query_in,
	
	// out (stream)
	hls::stream<ap_uint<512>>& s_fetched_vectors,
	hls::stream<int>& s_finish_query_out
) {

	for (int qid = 0; qid < query_num; qid++) {
		while (true) {
			// check query finish
			if (!s_finish_query_in.empty() && s_fetched_neighbor_ids_replicated.empty()) {
				s_finish_query_out.write(s_finish_query_in.read());
				break;
			} else if (!s_fetched_neighbor_ids_replicated.empty()) {
				// receive task
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

void results_collection(
	// in (initialization)
	const int query_num,
	// in runtime (stream)
	hls::stream<float>& s_distances_to_results_collection,
	hls::stream<int>& s_finish_query_in,

	// out (stream)
	hls::stream<int>& s_finish_query_out,
	
	// out (DRAM)
	float* out_id,
	float* out_dist
) {

	for (int qid = 0; qid < query_num; qid++) {
		while (true) {
			// check query finish
			if (!s_finish_query_in.empty() && s_distances_to_results_collection.empty()) {
				s_finish_query_out.write(s_finish_query_in.read());
				break;
			} else if (!s_distances_to_results_collection.empty()) {
				// receive task
				result_t reg_result = s_distances_to_results_collection.read();
				int node_id = reg_result.node_id;
				float distance = reg_result.dist;
			}
		}
	}
}