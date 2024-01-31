#pragma once

#include "types.hpp"

void task_scheduler(
	const int query_num, 
	const int d,
	const int max_level,
	const int max_link_num_top, 
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

	// out streams
	hls::stream<ap_uint<512>>& s_query_vectors,
	hls::stream<cand_t>& s_top_candidates,
	hls::stream<int>& s_finish_query_out
) {
	
	// similar to hsnwlin function `searchKnn`: https://github.com/nmslib/hnswlib/blob/master/hnswlib/hnswalg.h#L1271
	const int AXI_num_per_vector = d / FLOAT_PER_AXI; 
	bool first_iter_s_num_neighbors = true;
	bool first_iter_s_interm_results = true;
	bool first_iter_s_inserted_candidates = true;
	bool first_iter_s_largest_result_queue_elements = true;

	float entry_vector_buffer[D_MAX];
#pragma HLS unroll variable=entry_vector_buffer factor=float_per_axi

	float query_vector_buffer[D_MAX];
#pragma HLS unroll variable=query_vector_buffer factor=float_per_axi

	// TODO: maybe change this queue limit to alternative value? 
	Priority_queue<result_t, hardware_result_queue_size, Collect_smallest> candidate_queue(ef);
	const int sort_swap_round = ef % 2 == 0? ef / 2 : ef / 2 + 1;

	result_t queue_replication_array[hardware_queue_size];
#pragma HLS array_partition variable=queue_replication_array complete

	// read entry vector
	for (int i = 0; i < AXI_num_per_vector; i++) {
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

		// send out query vector
		int start_addr = qid * AXI_num_per_vector;
		for (int i = 0; i < AXI_num_per_vector; i++) {
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
		for (int i = 0; i < d / FLOAT_PER_AXI; i++) {
#pragma HLS pipeline II=1
			float partial_dist = 0;
			for (int s = 0; s < FLOAT_PER_AXI; s++) {
			#pragma HLS unroll	
				float diff = entry_vector_buffer[i * FLOAT_PER_AXI + s] - query_vector_buffer[i * FLOAT_PER_AXI + s];
				float partial_dist += diff * diff;
			}
			dist_entry_query += partial_dist;
		}

		int currObj = entry_point_id;
		float curdist = dist_entry_query;

		// search upper levels (top layer already computed)
        for (int level = max_level - 1; level > 0; level--) {
            bool changed = true;
            while (changed) {
                changed = false;
				// write out task
				cand_t reg_cand = {currObj, level};
				s_top_candidates.write(reg_cand);

				// receive the number of neighbors (each with a distance) to collect
				if (first_iter_s_num_neighbors) {
					while (s_num_neighbors_upper_levels.emtpy()) {}
					first_iter_s_num_neighbors = false;
				}
				int num_neighbors = s_num_neighbors_upper_levels.read();

				// update candidate
				if (first_iter_s_interm_results) {
					while (s_distances_upper_levels.emtpy()) {}
					first_iter_s_interm_results = false;
				}
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


		// search base layer
		candidate_queue.reset_queue(ef); // reset content to large_float
		int effect_queue_size = 0; // number of results in queue

		bool stop = false;
		while (!stop) {
			if (!s_num_inserted_candidates.empty()) {
				int num_inserted_candidates = s_num_inserted_candidates.read();

				if (first_iter_s_inserted_candidates) {
					while (s_inserted_candidates.empty()) {}
					first_iter_s_inserted_candidates = false;
				}

				// insert new values & sort
				for (int i = 0; i < num_inserted_candidates + sort_swap_round; i++) {
					if (i < num_inserted_candidates) {
						result_t reg = s_inserted_candidates.read();
						// if both input & queue element are large_float, then do not insert
						if (reg.dist < candidate_queue.queue[0].dist) {
							candidate_queue.queue[0] = reg;
						}
						candidate_queue.compare_swap_array_step_A();
						candidate_queue.compare_swap_array_step_B();
					}
				}

				// pop top candidate
				float threshold = s_largest_result_queue_elements.read();
				int smallest_element_position = ef - 1;
				if (smallest_element_position <= threshold) {
					// write new task
					cand_t reg_cand = {candidate_queue.queue[smallest_element_position].node_id, 0};
					s_top_candidates.write(reg_cand);

					// pop & right shift
					for (int i = 0; i < hardware_queue_size - 1; i++) {
					#pragma HLS UNROLL
						queue_replication_array[i] = candidate_queue.queue[i - 1];
					}
					candidate_queue.queue[0] = large_float;
					for (int i = 1; i < ef; i++) {
					#pragma HLS UNROLL
						candidate_queue.queue[i] = queue_replication_array[i];
					}
					for (int i = ef; i < hardware_queue_size; i++) {
					#pragma HLS UNROLL
						candidate_queue.queue[i] = -large_float;
					}
				} else {
					stop = true;
				}
			}

		}

		s_finish_query_out.write(qid);
	}
}
