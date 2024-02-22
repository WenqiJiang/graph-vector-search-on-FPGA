#pragma once

#include "types.hpp"

#include "types.hpp"

template <const int rep_factor>
void replicate_s_fetched_neighbor_ids(
	// in (initialization)
	const int query_num,
	// in (stream)
	hls::stream<cand_t>& s_fetched_neighbor_ids,
	hls::stream<int>& s_finish_query_in,
	
	// out (stream)
	hls::stream<cand_t> (&s_fetched_neighbor_ids_replicated)[rep_factor], 
	hls::stream<int>& s_finish_query_out
) {
	
	for (int qid = 0; qid < query_num; qid++) {
		while (true) {
			// check query finish
			if (!s_finish_query_in.empty() && s_fetched_neighbor_ids.empty()) {
				s_finish_query_out.write(s_finish_query_in.read());
				break;
			} else if (!s_fetched_neighbor_ids.empty()) {
				// receive task
				cand_t reg_cand = s_fetched_neighbor_ids.read();
				for (int i = 0; i < rep_factor; i++) {
				#pragma HLS UNROLL
					s_fetched_neighbor_ids_replicated[i].write(reg_cand);
				}
			}
		}

	}
}

// Send only upper-level results to results collection; replicate to other levels
void split_s_distances(
	// in (initialization)
	const int query_num,
	// in runtime (stream)
	hls::stream<result_t>& s_distances_filtered,
	hls::stream<int>& s_finish_query_in,

	// out (stream)
	hls::stream<result_t> &s_distances_upper_levels,
	hls::stream<result_t> &s_distances_base_level,
	hls::stream<int>& s_finish_query_out
) {

	for (int qid = 0; qid < query_num; qid++) {
		while (true) {
			// check query finish
			if (!s_finish_query_in.empty() && s_distances_filtered.empty()) {
				s_finish_query_out.write(s_finish_query_in.read());
				break;
			} else if (!s_distances_filtered.empty()) {
				// receive task
				result_t reg_out = s_distances_filtered.read();
				if (reg_out.level_id == 0) {
					s_distances_base_level.write(reg_out);
				} else {
					s_distances_upper_levels.write(reg_out);
				}
			}
		}
	}
}

// void check_visited(
// 	// in (initialization)
// 	const int query_num,
// 	// in runtime (stream)
// 	hls::stream<cand_t>& s_fetched_neighbor_ids_replicated,
// 	hls::stream<float>& s_distances,
// 	hls::stream<int>& s_finish_query_in,

// 	// out (stream)
// 	hls::stream<result_t>& s_distances_filtered,
// 	hls::stream<int>& s_finish_query_out
// ) {

// 	bool first_iter_s_distances = true;

// 	for (int qid = 0; qid < query_num; qid++) {
// 		while (true) {
// 			// check query finish
// 			if (!s_finish_query_in.empty() && s_fetched_neighbor_ids_replicated.empty() && s_distances.empty()) {
// 				s_finish_query_out.write(s_finish_query_in.read());

// 				// TODO: reset states (after writing out finish signal)
				
// 				break;
// 			}
// 			// task should arrive before distances (results), thus compute bloom filter first without waiting the results
// 			else if (!s_fetched_neighbor_ids_replicated.empty()) {
// 				// receive task
// 				cand_t reg_cand = s_fetched_neighbor_ids_replicated.read();
// 				int node_id = reg_cand.nodeID;
// 				int level = reg_cand.level;
// 				result_t out;
// 				out.node_id  = node_id;
// 				out.level_id = level;

// 				// wait distances
// 				if (first_iter_s_distances) {
// 					while (s_distances.empty()) {}
// 					first_iter_s_distances = false;
// 				}
// 				if (level == 0) {

// 					// TODO: compute bloom filter here
// 					unvisited = true;

// 					float distance = s_distances.read();
// 					if (unvisited) {
// 						out.dist = distance;
// 						s_distances_filtered.write(out);
// 					} else {
// 						out.dist = large_float;
// 						s_distances_filtered.write(out);
// 					}
// 				} else { // upper layer -> do not filter
// 					out.dist = s_distances.read();
// 					s_distances_filtered.write(out);
// 				}
// 			}
// 		}
// 	}
// }

