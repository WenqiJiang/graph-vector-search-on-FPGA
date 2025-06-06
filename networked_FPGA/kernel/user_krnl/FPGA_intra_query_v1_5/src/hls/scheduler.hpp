#pragma once

#include "types.hpp"

void task_scheduler(
	const int candidate_queue_runtime_size,
	const int max_cand_batch_size, 
	const int max_async_stage_num,

	// in streams
	hls::stream<int>& s_query_batch_size, // -1: stop
	hls::stream<ap_uint<512>>& s_query_vectors_in,	
	hls::stream<int>& s_entry_point_ids, 
	hls::stream<int>& s_num_inserted_candidates,
	hls::stream<result_t>& s_inserted_candidates,
	hls::stream<float>& s_largest_result_queue_elements,
	// hls::stream<int>& s_debug_num_vec_base_layer,
	hls::stream<int>& s_finish_query_in,

	// out streams
	hls::stream<ap_uint<512>> (&s_query_vectors_replicated)[N_CHANNEL],
	// hls::stream<result_t>& s_entry_point_base_level,
	hls::stream<int>& s_cand_batch_size, 
	hls::stream<cand_t>& s_top_candidates,
	hls::stream<int>& s_debug_signals,
	hls::stream<int>& s_finish_query_out
) {

	
	// similar to hsnwlin function `searchKnn`: https://github.com/nmslib/hnswlib/blob/master/hnswlib/hnswalg.h#L1271
	const int vec_AXI_num = D % FLOAT_PER_AXI == 0? D / FLOAT_PER_AXI : D / FLOAT_PER_AXI + 1; 
	bool first_s_query_batch_size = true;
	bool first_s_query_vectors = true;
	bool first_s_entry_point_ids = true;
	bool first_iter_s_inserted_candidates = true;
	bool first_iter_s_largest_result_queue_elements = true;
	// bool first_iter_s_debug_num_vec_base_layer = true;

	Priority_queue<result_t, hardware_candidate_queue_size, Collect_smallest> candidate_queue(candidate_queue_runtime_size);
	const int sort_swap_round = candidate_queue_runtime_size % 2 == 0? candidate_queue_runtime_size / 2 : candidate_queue_runtime_size / 2 + 1;

	result_t queue_replication_array[hardware_candidate_queue_size];
#pragma HLS array_partition variable=queue_replication_array complete

	int debug_hops_base_layer;
	while (true) {

		wait_data_fifo_first_iter<int>(
			1, s_query_batch_size, first_s_query_batch_size);
		int query_num = s_query_batch_size.read();
		if (query_num == -1) {
			break;
		}

		for (int qid = 0; qid < query_num; qid++) {

			// send out query vector
			wait_data_fifo_first_iter<ap_uint<512>>(
				1, s_query_vectors_in, first_s_query_vectors);
			for (int i = 0; i < vec_AXI_num; i++) {
			#pragma HLS pipeline II=1
				ap_uint<512> query_vector_AXI = s_query_vectors_in.read();
				for (int cid = 0; cid < N_CHANNEL; cid++) {
				#pragma HLS unroll
					s_query_vectors_replicated[cid].write(query_vector_AXI);
				}
			}

			// search base layer
			candidate_queue.reset_queue(); // reset content to large_float
			int effect_queue_size = 0; // number of results in queue
			// first task
			wait_data_fifo_first_iter<int>(
				1, s_entry_point_ids, first_s_entry_point_ids);
			int entry_point_id = s_entry_point_ids.read();
			s_top_candidates.write({entry_point_id, 0});

			int last_cand_to_be_recv_batch_size = 1; // the size of the last batch of popped candidates
			s_cand_batch_size.write(last_cand_to_be_recv_batch_size);

			int on_the_fly_async_stage_num = 1;

			// Note: entry point must not be sent to the result queue: if the entry point
			//   is close to the query, the neighbors of entry point will be inserted into the queue, which will then search entry,
			//   adding entry to the result immediately will lead to visit tag mismatch as entry can be inserted twice into the result queue
			// s_entry_point_base_level.write({currObj, 0, curdist}); 

			debug_hops_base_layer = 1;

			// regardless of batch size, pull one time from each channel
			int recv_channels_left = N_CHANNEL;

			bool stop = false;
			while (!stop) {
				if (!s_num_inserted_candidates.empty()) { // each channel send a number, regardless of per-iter batch size

					int num_insertion = s_num_inserted_candidates.read();
					wait_data_fifo_first_iter<result_t>(
						num_insertion, s_inserted_candidates, first_iter_s_inserted_candidates);
					// insert new values
					candidate_queue.insert_only(num_insertion, s_inserted_candidates);
					recv_channels_left--; // always loading the latest-sent batch 
					
					// if all last batch of candidates are consumed, sort & pop top candidate
					if (recv_channels_left == 0) {
						// finish a on-the-fly batch, shift the batch_size in the array
						on_the_fly_async_stage_num--;
						candidate_queue.sort();

						// two stop condition: 1. smallest candidate distance > largest result queue element; 
						//  2. candidate queue is empty (which also means the first condition is satisfied), so only need to check (1)
						wait_data_fifo_first_iter<float>(
							1, s_largest_result_queue_elements, first_iter_s_largest_result_queue_elements);
						float threshold = s_largest_result_queue_elements.read();
						const int smallest_element_position = candidate_queue_runtime_size - 1;
						
						// send tasks until filling the pipeline
						for (int oid = on_the_fly_async_stage_num; oid < max_async_stage_num; oid++) {
							int current_cand_batch_size = 0;
							for (int bid = 0; bid < max_cand_batch_size; bid++) {
								if (candidate_queue.queue[smallest_element_position].dist <= threshold &&
									candidate_queue.queue[smallest_element_position].dist < large_float) {
									candidate_queue.pop_top(s_top_candidates);
									debug_hops_base_layer++;
									current_cand_batch_size++;
								} else {
									break;
								}
							}
							if (current_cand_batch_size == 0) {
								break;
							} else { // current_cand_batch_size > 0
								s_cand_batch_size.write(current_cand_batch_size);
								on_the_fly_async_stage_num++;
							}
						}

						// reset next num batch to receive
						recv_channels_left = N_CHANNEL;

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
			// debug_num_vec_base_layer = s_debug_num_vec_base_layer.read();

			s_debug_signals.write(debug_hops_base_layer);
			while (s_finish_query_in.empty()) {}
			int finish_query_in = s_finish_query_in.read();
		}
	}
}
