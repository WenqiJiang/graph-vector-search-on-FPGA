#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "types.hpp"

extern "C" {

void vadd(  
	// in initialization
	const int query_num, 
	const int d,
	const int max_level,
    const int max_link_num_top, 
	const int max_link_num_base,
	const int entry_point_id,
    // in runtime (should from DRAM)
	const ap_uint<512>* query_vectors,
    const ap_uint<512>* links_top,
    const ap_uint<512>* links_base,
    const ap_uint<512>* vectors_top,
    const ap_uint<512>* vectors_base,
	   
    // out
    int* out_id,
	float* out_dist
    )
{
// Share the same AXI interface with several control signals (but they are not allowed in same dataflow)
//    https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Controlling-AXI4-Burst-Behavior

// in runtime (should from DRAM)
#pragma HLS INTERFACE m_axi port=query_vectors offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=links_top offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=links_base offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=vectors_top offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=vectors_base offset=slave bundle=gmem4

// out
#pragma HLS INTERFACE m_axi port=out_id  offset=slave bundle=gmem9
#pragma HLS INTERFACE m_axi port=out_dist  offset=slave bundle=gmem10

#pragma HLS dataflow

    hls::stream<int> s_finish_query_task_scheduler; // finish the current query
#pragma HLS stream variable=s_finish_query_task_scheduler depth=2

    hls::stream<int> s_num_neighbors; // number of neighbors of the current candidate
#pragma HLS stream variable=s_num_neighbors depth=512
	
	hls::stream<ap_uint<512>> s_query_vectors;
#pragma HLS stream variable=s_query_vectors depth=128

    hls::stream<cand_t> s_top_candidates; // current top candidates
#pragma HLS stream variable=s_top_candidates depth=512
 
	hls::stream<result_t> s_interm_results;
#pragma HLS stream variable=s_interm_results depth=512

	// controls the traversal and maintains the candidate queue
	task_scheduler(
		query_num, 
		d, 
		max_level
    	max_link_num_top, 
		max_link_num_base,
		entry_point_id,
    	// in runtime (should from DRAM)
		query_vectors,
		// in streams
		s_num_neighbors, 
		s_interm_results,
		// out streams
		s_query_vectors,
		s_top_candidates,
		s_finish_query_task_scheduler
	);
	

    hls::stream<cand_t> s_fetched_neighbor_ids; 
#pragma HLS stream variable=s_fetched_neighbor_ids depth=512

    hls::stream<int> s_finish_query_fetch_neighbor_ids; // finish all queries
#pragma HLS stream variable=s_finish_query_fetch_neighbor_ids depth=2

	fetch_neighbor_ids(
		// in initialization
		query_num,
    	max_link_num_top, 
		max_link_num_base,
		// in runtime (should from DRAM)
    	links_top,
    	links_base,
		// in runtime (stream)
		s_top_candidates,
		s_finish_query_task_scheduler,

		// out (stream)
		s_num_neighbors,
		s_fetched_neighbor_ids,
		s_finish_query_fetch_neighbor_ids
	);

	const int replication_factor_s_fetched_neighbor_ids = 2;

    hls::stream<cand_t> s_fetched_neighbor_ids_replicated[replication_factor_s_fetched_neighbor_ids]; 
#pragma HLS stream variable=s_fetched_neighbor_ids_replicated depth=512

    hls::stream<int> s_finish_query_replicate_s_fetched_neighbor_ids; // finish all queries
#pragma HLS stream variable=s_finish_query_replicate_s_fetched_neighbor_ids depth=2

	replicate_s_fetched_neighbor_ids<replication_factor_s_fetched_neighbor_ids>(
		// in (initialization)
		query_num,
		// in (stream)
		s_fetched_neighbor_ids,
		s_finish_query_fetch_neighbor_ids,
		
		// out (stream)
		s_fetched_neighbor_ids_replicated, 
		s_finish_query_replicate_s_fetched_neighbor_ids
	);


    hls::stream<float> s_fetched_vectors[FLOAT_PER_AXI]; 
#pragma HLS stream variable=s_fetched_vectors depth=512
//     hls::stream<int> s_num_fetched_vectors; // fetched number of vectors per node
// #pragma HLS stream variable=s_num_fetched_vectors depth=512

    hls::stream<int> s_finish_query_fetch_vectors; // finish all queries
#pragma HLS stream variable=s_finish_query_fetch_vectors depth=2

	fetch_vectors(
		// in initialization
		query_num,
		d,
		// in runtime (should from DRAM)
    	vectors_top,
    	vectors_base,
		// in runtime (stream)
		s_fetched_neighbor_ids_replicated[0],
		s_finish_query_replicate_s_fetched_neighbor_ids,
		
		// out (stream)
		s_fetched_vectors,
		s_finish_query_fetch_vectors
	);

    hls::stream<float> s_fetched_vectors_filtered[FLOAT_PER_AXI]; 
#pragma HLS stream variable=s_fetched_vectors_filtered depth=512
	
    hls::stream<int> s_finish_query_check_visited; // finish all queries
#pragma HLS stream variable=s_finish_query_check_visited depth=2

	check_visited(
		// in (initialization)
		query_num,
		d,
		// in runtime (stream)
		s_fetched_neighbor_ids_replicated[1],
		s_fetched_vectors,
		s_finish_query_fetch_vectors,

		// out (stream)
		s_fetched_vectors_filtered,
		s_finish_query_check_visited
	);
	
    hls::stream<float> s_distances; 
#pragma HLS stream variable=s_distances depth=512

    hls::stream<int> s_finish_query_compute_distances; // finish all queries
#pragma HLS stream variable=s_finish_query_compute_distances depth=2

	compute_distances(
		// in initialization
		query_num,
		d,
		// in runtime (stream)
		s_query_vectors,
		s_fetched_vectors_filtered,
		s_finish_query_check_visited,
		
		// out (stream)
		s_distances,
		s_finish_query_compute_distances
	);

	hls::stream<float> s_distances_replicated[2];
#pragma HLS stream variable=s_distances_replicated depth=512

	hls::stream<int> s_finish_query_replicate_distances; // finish all queries
#pragma HLS stream variable=s_finish_query_replicate_distances depth=2

	replicate_s_distances(
		// in (initialization)
		query_num,
		// in runtime (stream)
		s_distances,
		s_finish_query_compute_distances,

		// out (stream)
		s_distances_replicated,
		s_finish_query_replicate_distances
	);


//     hls::stream<int> s_finish_query_results_collection; // finish all queries
// #pragma HLS stream variable=s_finish_query_results_collection depth=2

	results_collection(
		// in (initialization)
		query_num,
		// in runtime (stream)
		s_distances_replicated[0],
		s_finish_query_replicate_distances,

		// out (stream)
		// s_finish_query_results_collection
		// out (DRAM)
		out_id,
		out_dist
	);
}

}
