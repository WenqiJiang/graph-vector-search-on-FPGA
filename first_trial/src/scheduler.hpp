#pragma once

#include "types.hpp"

void task_scheduler(
	const int query_num, 
	const int d,
	const int max_level,
	const int max_link_num_top, 
	const int max_link_num_base,
	const int entry_point_id,
	// in runtime (should from DRAM)
	const ap_uint<512>* query_vectors,
	// in streams
	hls::stream<int>& s_num_neighbors,
	hls::stream<result_t>& s_interm_results,
	// out streams
	hls::stream<ap_uint<512>>& s_query_vectors,
	hls::stream<cand_t>& s_top_candidates,
	hls::stream<int>& s_finish_query_task_scheduler
) {
	
	// similar to hsnwlin function `searchKnn`: https://github.com/nmslib/hnswlib/blob/master/hnswlib/hnswalg.h#L1271
	const int vec_AXI_num = d / FLOAT_PER_AXI; 
	bool first_iter_s_num_neighbors = true;
	bool first_iter_s_interm_results = true;

	for (int qid = 0; qid < query_num; qid++) {
	
		// send out query vector
		int start_addr = qid * vec_AXI_num;
		for (int i = 0; i < vec_AXI_num; i++) {
		#pragma HLS pipeline II=1
			ap_uint<512> query_vector_AXI = query_vectors[start_addr + i];
			s_query_vectors.write(query_vector_AXI);
		}

		// Wenqi comments: in top level, the neighbor number has to be set as 1, i.e., the entry point, such that 
		///  we can compute the distance
		// search upper levels
		int currObj = entry_point_id;
		float curdist = large_float;

        for (int level = max_level; level > 0; level--) {
            bool changed = true;
            while (changed) {
                changed = false;
				// write out task
				cand_t reg_cand = {currObj, level};
				s_top_candidates.write(reg_cand);

				// receive the number of neighbors (each with a distance) to collect
				if (first_iter_s_num_neighbors) {
					while (s_num_neighbors.emtpy()) {}
					first_iter_s_num_neighbors = false;
				}
				int num_neighbors = s_num_neighbors.read();

				// update candidate
				if (first_iter_s_interm_results) {
					while (s_interm_results.emtpy()) {}
					first_iter_s_interm_results = false;
				}
				for (int i = 0; i < num_neighbors; i++) {
					result_t interm_result = s_interm_results.read();
                    if (interm_result.dist < curdist) {
                        curdist = d;
                        currObj = interm_result.node_id;
                        changed = true;
                    }
                }

				// top-level only evaluate one point (update distance), then break
				if (level == max_level) { 
					break; 
				}
            }
        }

		// TODO: search base layer
		// do {

		// } while (true);
	}
}