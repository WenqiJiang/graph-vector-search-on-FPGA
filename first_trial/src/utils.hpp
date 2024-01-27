#pragma once

#include "types.hpp"

#include "types.hpp"

template <const int rep_factor>
void replicate_s_fetched_neighbor_ids(
	// in (initialization)
	const int query_num,
	// in (stream)
	hls::stream<cand_t>& s_fetched_neighbor_ids,
	hls::stream<int>& s_finish_query_fetch_neighbor_ids,
	
	// out (stream)
	hls::stream<cand_t> (&s_fetched_neighbor_ids_replicated)[rep_factor], 
	hls::stream<int>& s_finish_query_replicate_s_fetched_neighbor_ids
) {

	bool first_iter_s_fetched_neighbor_ids = true;
	// bool first_iter_s_finish_query_task_scheduler = true;
	
	for (int qid = 0; qid < query_num; qid++) {
		while (true) {
			// check query finish
			if (!s_finish_query_fetch_neighbor_ids.empty() && s_fetched_neighbor_ids_replicated.empty()) {
				s_finish_query_replicate_s_fetched_neighbor_ids.write(s_finish_query_fetch_neighbor_ids.read());
				break;
			} else {
				// receive task
				if (first_iter_s_fetched_neighbor_ids) {
					while (s_fetched_neighbor_ids.empty()) {}
					first_iter_s_fetched_neighbor_ids = false;
				}
				cand_t reg_cand = s_fetched_neighbor_ids.read();
				for (int i = 0; i < rep_factor; i++) {
				#pragma HLS UNROLL
					s_fetched_neighbor_ids_replicated[i].write(reg_cand);
				}
			}
		}

	}
}

void replicate_s_distances(
	// in (initialization)
	const int query_num,
	// in runtime (stream)
	hls::stream<float>& s_distances,
	hls::stream<int>& s_finish_query_compute_distances,

	// out (stream)
	hls::stream<float>& s_distances_replicated,
	hls::stream<int>& s_finish_query_replicate_distances
) {
	
}
