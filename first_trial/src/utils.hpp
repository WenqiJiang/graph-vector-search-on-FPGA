#pragma once

#include "types.hpp"

#include "types.hpp"

void replicate_s_fetched_neighbor_ids(
	// in (initialization)
	const int query_num,
	// in (stream)
	hls::stream<int>& s_fetched_neighbor_ids,
	hls::stream<int>& s_finish_query_fetch_neighbor_ids,
	
	// out (stream)
	hls::stream<int>& s_fetched_neighbor_ids_replicated, 
	hls::stream<int>& s_finish_query_replicate_s_fetched_neighbor_ids
) {

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
