#include "bloom_filter.hpp"
#include "bloom_fetch_compute.hpp"
#include "compute.hpp"
#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "scheduler.hpp"
#include "types.hpp"
#include "utils.hpp"

extern "C" {

// Wrapping the processing logic for each channel
void per_channel_processing_wrapper(
	// in initialization
	const int ef, // size of the result priority queue
	const int candidate_queue_runtime_size, 
	const int max_cand_batch_size, 
	const int max_async_stage_num,
	const int runtime_n_bucket_addr_bits,
	const ap_uint<32> hash_seed,
	const int max_bloom_out_burst_size,
	const int max_link_num_base,

    // in runtime (from DRAM)
    ap_uint<512>* db_vectors,
    const ap_uint<512>* links_base,	

	// in streams
	hls::stream<int>& axis_query_batch_size, // -1: stop
	hls::stream<ap_uint<512>>& axis_query_vectors_in,	
	hls::stream<int>& axis_entry_point_ids, 

	// out streams
	hls::stream<int>& axis_out_ids,
	hls::stream<float>& axis_out_dists,
	hls::stream<int>& axis_debug_signals
) {

// DRAM
#pragma HLS INTERFACE m_axi port=db_vectors latency=64 num_read_outstanding=64  num_write_outstanding=1 max_write_burst_length=2 offset=slave bundle=gmemdb_vectors
#pragma HLS INTERFACE m_axi port=links_base latency=32 num_read_outstanding=16  num_write_outstanding=1 max_write_burst_length=2 offset=slave bundle=gmemlinks_base

// in streams
#pragma HLS INTERFACE axis port=axis_query_batch_size
#pragma HLS INTERFACE axis port=axis_query_vectors_in
#pragma HLS INTERFACE axis port=axis_entry_point_ids

// out streams
#pragma HLS INTERFACE axis port=axis_out_ids
#pragma HLS INTERFACE axis port=axis_out_dists
#pragma HLS INTERFACE axis port=axis_debug_signals


#pragma HLS dataflow

    hls::stream<int> s_finish_query_task_scheduler; // finish the current query
#pragma HLS stream variable=s_finish_query_task_scheduler depth=16

    hls::stream<int> s_finish_query_results_collection; // finish all queries
#pragma HLS stream variable=s_finish_query_results_collection depth=16
	
	hls::stream<ap_uint<512>> s_query_vectors;
#pragma HLS stream variable=s_query_vectors depth=128

// 	hls::stream<result_t> s_entry_point_base_level;
// #pragma HLS stream variable=s_entry_point_base_level depth=16

	hls::stream<int> s_cand_batch_size;
#pragma HLS stream variable=s_cand_batch_size depth=16

    hls::stream<cand_t> s_top_candidates; // current top candidates
#pragma HLS stream variable=s_top_candidates depth=512

	hls::stream<int> s_num_inserted_candidates;
#pragma HLS stream variable=s_num_inserted_candidates depth=512

	hls::stream<result_t> s_inserted_candidates;
#pragma HLS stream variable=s_inserted_candidates depth=512

	hls::stream<int> s_num_valid_candidates_base_level_total;
#pragma HLS stream variable=s_num_valid_candidates_base_level_total depth=16

	hls::stream<result_t> s_distances_base_level;
#pragma HLS stream variable=s_distances_base_level depth=512

	hls::stream<float> s_largest_result_queue_elements;
#pragma HLS stream variable=s_largest_result_queue_elements depth=512	
	
	hls::stream<int> s_debug_num_vec_base_layer;
#pragma HLS stream variable=s_debug_num_vec_base_layer depth=16

	// replicate axis_query_batch_size to multiple streams
	const int replicate_factor_s_query_batch_size = 4;
	hls::stream<int> s_query_batch_size_replicated[replicate_factor_s_query_batch_size];
#pragma HLS stream variable=s_query_batch_size_replicated depth=16

	replicate_s_query_batch_size<replicate_factor_s_query_batch_size>(
		axis_query_batch_size,
		s_query_batch_size_replicated
	);

	// controls the traversal and maintains the candidate queue
	task_scheduler(
		candidate_queue_runtime_size,
		max_cand_batch_size,
		max_async_stage_num,

		// in streams
		s_query_batch_size_replicated[0], // -1: stop
		axis_query_vectors_in,
		axis_entry_point_ids,
		s_num_inserted_candidates,
		s_inserted_candidates,
		s_largest_result_queue_elements,
		s_debug_num_vec_base_layer,
		s_finish_query_results_collection,
		
		// out streams
		s_query_vectors,
		// s_entry_point_base_level,
		s_cand_batch_size,
		s_top_candidates,
		axis_debug_signals,
		s_finish_query_task_scheduler
	);

	hls::stream<int> s_fetch_batch_size;
#pragma HLS stream variable=s_fetch_batch_size depth=16
	
    hls::stream<int> s_num_neighbors_base_level; // number of neighbors of the current candidate
#pragma HLS stream variable=s_num_neighbors_base_level depth=512

    hls::stream<cand_t> s_fetched_neighbor_ids; 
#pragma HLS stream variable=s_fetched_neighbor_ids depth=512

    hls::stream<int> s_finish_query_fetch_neighbor_ids; // finish all queries
#pragma HLS stream variable=s_finish_query_fetch_neighbor_ids depth=16

	fetch_neighbor_ids(
		// in initialization
		max_link_num_base,
		// in runtime (should from DRAM)
    	links_base,
		// in runtime (stream)
		s_query_batch_size_replicated[1],
		s_top_candidates,
		s_finish_query_task_scheduler,

		// out (stream)
		s_num_neighbors_base_level,
		s_fetched_neighbor_ids,
		s_finish_query_fetch_neighbor_ids
	);


    hls::stream<int> s_finish_query_bloom_fetch_compute; // finish all queries
#pragma HLS stream variable=s_finish_query_bloom_fetch_compute depth=16

	bloom_fetch_compute(
		// in initialization
		runtime_n_bucket_addr_bits,
		hash_seed,
		max_bloom_out_burst_size,

		// in runtime (from DRAM)
		db_vectors, // need to write visited tag

		// in streams
		s_query_batch_size_replicated[2], // -1: stop
		s_query_vectors,
		s_num_neighbors_base_level,
		s_fetched_neighbor_ids,
		s_finish_query_fetch_neighbor_ids,

		// out streams
		s_num_valid_candidates_base_level_total,
		s_distances_base_level,
		s_finish_query_bloom_fetch_compute
	);

	results_collection(
		// in (initialization)
		ef,
		// in runtime (stream)
		s_query_batch_size_replicated[3],
		// s_entry_point_base_level,
		s_cand_batch_size,
		s_num_valid_candidates_base_level_total,
		s_distances_base_level,
		s_finish_query_bloom_fetch_compute,

		// out (stream)
		s_inserted_candidates,
		s_num_inserted_candidates,
		s_largest_result_queue_elements,
		s_debug_num_vec_base_layer,
		s_finish_query_results_collection,
		axis_out_ids,
		axis_out_dists
	);

}

}