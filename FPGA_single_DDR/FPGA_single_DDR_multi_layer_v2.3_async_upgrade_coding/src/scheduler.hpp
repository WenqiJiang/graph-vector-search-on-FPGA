#pragma once

#include "types.hpp"

void task_scheduler(
	const int query_num, 
	const int candidate_queue_runtime_size,
	const int max_cand_batch_size, 
	const int max_async_stage_num,
	const int d,
	const int max_level,
	const int max_link_num_upper, 
	const int max_link_num_base,
	const int entry_point_id,

	// in initialization (from DRAM)
	const ap_uint<512>* entry_vector, 
	// in runtime (should from DRAM)
	const ap_uint<512>* query_vectors,
	// in streams
	hls::stream<int>& s_num_neighbors_upper_levels,
	hls::stream<result_t>& s_distances_upper_levels,
	hls::stream<int>& s_num_inserted_candidates,
	hls::stream<result_t>& s_inserted_candidates,
	hls::stream<float>& s_largest_result_queue_elements,
	// hls::stream<int>& s_debug_num_vec_base_layer,
	hls::stream<int>& s_finish_query_in,

	// out streams
	hls::stream<ap_uint<512>>& s_query_vectors,
	// hls::stream<result_t>& s_entry_point_base_level,
	hls::stream<int>& s_cand_batch_size, 
	hls::stream<cand_t>& s_top_candidates,
	hls::stream<int>& s_finish_query_out,

	// debug signals (each 4 byte): 
	//   0: bottom layer entry node id, 
	//   1: number of hops in upper layers 
	//   2: number of read vectors in upper layers
	//   3: number of hops in base layer (number of pop operations)
	//   4: number of valid read vectors in base layer
	int* mem_debug
) {

	
	// similar to hsnwlin function `searchKnn`: https://github.com/nmslib/hnswlib/blob/master/hnswlib/hnswalg.h#L1271
	const int vec_AXI_num = d % FLOAT_PER_AXI == 0? d / FLOAT_PER_AXI : d / FLOAT_PER_AXI + 1; 
	bool first_iter_s_num_neighbors_upper_levels = true;
	bool first_iter_s_distances_upper_levels = true;
	bool first_iter_s_inserted_candidates = true;
	bool first_iter_s_largest_result_queue_elements = true;
	// bool first_iter_s_debug_num_vec_base_layer = true;

	float entry_vector_buffer[D_MAX];
#pragma HLS unroll variable=entry_vector_buffer factor=float_per_axi
#pragma HLS bind_storage variable=entry_vector_buffer type=RAM_2P impl=BRAM

	float query_vector_buffer[D_MAX];
#pragma HLS unroll variable=query_vector_buffer factor=float_per_axi
#pragma HLS bind_storage variable=query_vector_buffer type=RAM_2P impl=BRAM

	Priority_queue<result_t, hardware_candidate_queue_size, Collect_smallest> candidate_queue(candidate_queue_runtime_size);
	const int sort_swap_round = candidate_queue_runtime_size % 2 == 0? candidate_queue_runtime_size / 2 : candidate_queue_runtime_size / 2 + 1;

	result_t queue_replication_array[hardware_candidate_queue_size];
#pragma HLS array_partition variable=queue_replication_array complete

			int async_batch_size_array[hardware_async_batch_size];
#pragma HLS bind_storage variable=async_batch_size_array type=RAM_2P impl=BRAM

	const int debug_size = 1;
	// const int debug_size = 5;
	// int debug_signals[debug_size];
	// int* debug_bottom_entry_id = &debug_signals[0];
	// int* debug_hops_upper_layers = &debug_signals[1];
	// int* debug_num_vec_upper_layers = &debug_signals[2];
	// int* debug_hops_base_layer = &debug_signals[3];
	// int* debug_num_vec_base_layer = &debug_signals[4];
	int debug_hops_base_layer;

	// read entry vector
	for (int i = 0; i < vec_AXI_num; i++) {
	#pragma HLS pipeline II=1
		ap_uint<512> entry_vector_AXI = entry_vector[i];
		for (int j = 0; j < FLOAT_PER_AXI; j++) {
		#pragma HLS unroll
			ap_uint<32> entry_vector_uint32 = entry_vector_AXI.range(32 * (j + 1) - 1, 32 * j);
			float entry_vector_float = *((float*) (&entry_vector_uint32));
			entry_vector_buffer[i * FLOAT_PER_AXI + j] = entry_vector_float;
		}
	}

	for (int qid = 0; qid < query_num; qid++) {

		// here, make sure do not start the next query before the current query if fully ended,
		//   because the query termination condition of other PEs is that finish signal arrives && data FIFOs are empty
		if (qid > 0) {
			while (s_finish_query_in.empty()) {}
			int finish_query_in = s_finish_query_in.read();
		}
		// for (int did = 0; did < debug_size; did++) {
		// 	debug_signals[did] = 0;
		// }

		// send out query vector
		int start_addr = qid * vec_AXI_num;
		for (int i = 0; i < vec_AXI_num; i++) {
		#pragma HLS pipeline II=1
			ap_uint<512> query_vector_AXI = query_vectors[start_addr + i];
			s_query_vectors.write(query_vector_AXI);

			for (int j = 0; j < FLOAT_PER_AXI; j++) {
			#pragma HLS unroll
				ap_uint<32> query_vector_uint32 = query_vector_AXI.range(32 * (j + 1) - 1, 32 * j);
				float query_vector_float = *((float*) (&query_vector_uint32));
				query_vector_buffer[i * FLOAT_PER_AXI + j] = query_vector_float;
			}
		}

		// compute distance between entry and query
		float dist_entry_query = 0;
		for (int i = 0; i < vec_AXI_num; i++) {
		#pragma HLS pipeline II=1
			float partial_dist = 0;
			for (int s = 0; s < FLOAT_PER_AXI; s++) {
			#pragma HLS unroll	
				float diff = entry_vector_buffer[i * FLOAT_PER_AXI + s] - query_vector_buffer[i * FLOAT_PER_AXI + s];
				partial_dist += diff * diff;
			}
			dist_entry_query += partial_dist;
		}

		int currObj = entry_point_id;
		float curdist = dist_entry_query;

		// search upper levels (top layer already computed, and it has 0 links, 
		//   so starting from max_level will lead to deadlock, differing from the sw version which can handle 0 links)
		// *debug_hops_upper_layers = 1;
		// *debug_num_vec_upper_layers = 1;
        for (int level = max_level - 1; level > 0; level--) {
            bool changed = true;
            while (changed) {
                changed = false;
				// write out task
				cand_t reg_cand = {currObj, level};
				s_top_candidates.write(reg_cand);
				// (*debug_hops_upper_layers)++;

				// receive the number of neighbors (each with a distance) to collect
				wait_data_fifo_first_iter<int>(
					1, s_num_neighbors_upper_levels, first_iter_s_num_neighbors_upper_levels);
				int num_neighbors = s_num_neighbors_upper_levels.read();
				// *debug_num_vec_upper_layers += num_neighbors;
				
				// update candidate
				wait_data_fifo_first_iter<result_t>(
					num_neighbors, s_distances_upper_levels, first_iter_s_distances_upper_levels);
				for (int i = 0; i < num_neighbors; i++) {
					result_t interm_result = s_distances_upper_levels.read();
                    if (interm_result.dist < curdist) {
                        curdist = interm_result.dist;
                        currObj = interm_result.node_id;
                        changed = true;
                    }
                }
            }
        }

		// *debug_bottom_entry_id = currObj;

		// search base layer
		candidate_queue.reset_queue(); // reset content to large_float
		int effect_queue_size = 0; // number of results in queue
		// first task
		s_top_candidates.write({currObj, 0});

		int last_cand_to_be_recv_batch_size = 1; // the size of the last batch of popped candidates
		async_batch_size_array[0] = last_cand_to_be_recv_batch_size;
		s_cand_batch_size.write(last_cand_to_be_recv_batch_size);

		int on_the_fly_async_stage_num = 1;

		// Note: entry point must not be sent to the result queue: if the entry point
		//   is close to the query, the neighbors of entry point will be inserted into the queue, which will then search entry,
		//   adding entry to the result immediately will lead to visit tag mismatch as entry can be inserted twice into the result queue
		// s_entry_point_base_level.write({currObj, 0, curdist}); 

		// *debug_hops_base_layer = 1;
		debug_hops_base_layer = 1;

		bool stop = false;
		while (!stop) {
			if (!s_num_inserted_candidates.empty()) {

				int num_insertion = s_num_inserted_candidates.read();
				wait_data_fifo_first_iter<result_t>(
					num_insertion, s_inserted_candidates, first_iter_s_inserted_candidates);
				// insert new values
				candidate_queue.insert_only(num_insertion, s_inserted_candidates);
				async_batch_size_array[0]--; // always loading the latest-sent batch 
				
				// if all last batch of candidates are consumed, sort & pop top candidate
				if (async_batch_size_array[0] == 0) {
					wait_data_fifo_first_iter<float>(
						1, s_largest_result_queue_elements, first_iter_s_largest_result_queue_elements);
					// finish a on-the-fly batch, shift the batch_size in the array
					on_the_fly_async_stage_num--;
					for (int i = 0; i < on_the_fly_async_stage_num; i++) {
						async_batch_size_array[i] = async_batch_size_array[i + 1];
					}

					candidate_queue.sort();

					// two stop condition: 1. smallest candidate distance > largest result queue element; 
					//  2. candidate queue is empty (which also means the first condition is satisfied), so only need to check (1)
					float threshold = s_largest_result_queue_elements.read();
					const int smallest_element_position = candidate_queue_runtime_size - 1;
					
					// send task until filling the pipeline
					for (int oid = on_the_fly_async_stage_num; oid < max_async_stage_num; oid++) {
						int current_cand_batch_size = 0;
						for (int bid = 0; bid < max_cand_batch_size; bid++) {
							if (candidate_queue.queue[smallest_element_position].dist <= threshold &&
								candidate_queue.queue[smallest_element_position].dist < large_float) {
								candidate_queue.pop_top(s_top_candidates);
								// (*debug_hops_base_layer)++;
								debug_hops_base_layer++;
								current_cand_batch_size++;
							} else {
								break;
							}
						}
						if (current_cand_batch_size == 0) {
							break;
						} else { // current_cand_batch_size > 0
							async_batch_size_array[oid] = current_cand_batch_size;
							s_cand_batch_size.write(current_cand_batch_size);
							on_the_fly_async_stage_num++;
						}
					}

					// break condition: nothing in the pipeline even after candidate popping	
					if (on_the_fly_async_stage_num == 0) {
						stop = true;
					}
				}
			}
		}

		s_finish_query_out.write(qid);

		// wait_data_fifo_first_iter<int>(
		// 	1, s_debug_num_vec_base_layer, first_iter_s_debug_num_vec_base_layer);
		// *debug_num_vec_base_layer = s_debug_num_vec_base_layer.read();

		// for (int did = 0; did < debug_size; did++) {
		// #pragma HLS pipeline II=1
		// 	mem_debug[qid * debug_size + did] = debug_signals[did];
		// }

		mem_debug[qid * debug_size] = debug_hops_base_layer;
	}

	while (s_finish_query_in.empty()) {}
	int finish_query_in = s_finish_query_in.read();
}
