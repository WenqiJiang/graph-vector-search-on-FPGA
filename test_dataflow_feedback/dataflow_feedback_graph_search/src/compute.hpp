#pragma once

#include "types.hpp"

void compute_distances(
	// in initialization
	const int query_num,
	const int d,
	// in runtime (stream)
    hls::stream<float>& s_query_vector, 
	hls::stream<float>& s_fetched_vectors_filtered[FLOAT_PER_AXI],
	hls::stream<int>& s_finish_query_check_visited,
	
	// out (stream)
	hls::stream<float>& s_distances,
	hls::stream<int>& s_finish_query_compute_distances
) {

	const int vec_AXI_num = d / FLOAT_PER_AXI; 
    
	
}

void compute_per_AXI(
    // in initialization
    const int query_num,
    const int d,
    // in runtime (stream)
    hls::stream<float>& s_query_vector, 
    hls::stream<float>& s_fetched_vectors_filtered[FLOAT_PER_AXI],
    hls::stream<int>& s_finish_query_check_visited,
    
    // out (stream)
    hls::stream<float>& s_partial_distances,
    hls::stream<int>& s_finish_query_internal
) {
    // Wenqi comments: future upgrade -> less check on query finish, this will delay the progress
    //  TOOD: add a signal: everytime we check finish, check how many more computations should the unit expect

    float query_vector[D_MAX];
#pragma HLS unroll variable=query_vector factor=float_per_axi

    for (int qid = 0; qid < query_num; qid++) {

        // read query vector
        for (int i = 0; i < d; i++) {
            query_vector[i] = s_query_vector.read();
        }

        while (true) {
            
            if (!s_finish_query_check_visited.empty() && s_fetched_vectors_filtered[0].empty()) {
                s_finish_query_internal.write(s_finish_query_check_visited.read());
                break;
            } else {
                for (int i = 0; i < vec_AXI_num; i++) {
                #pragma HLS pipeline II=1
                    float database_vector_partial[FLOAT_PER_AXI];
                    for int (int s = 0; s < FLOAT_PER_AXI; s++) {
                    #pragma HLS unroll
                        database_vector_partial[s] = s_fetched_vectors_filtered[s].read();
                    }
                    // compute L2 distance
                    float distance_partial = 0;
                    for (int s = 0; s < FLOAT_PER_AXI; s++) {
                    #pragma HLS unroll
                        float diff = query_vector[i * FLOAT_PER_AXI + s] - database_vector_partial[s];
                        distance_partial += diff * diff;
                    }
                }
            }
        }
    }
}
