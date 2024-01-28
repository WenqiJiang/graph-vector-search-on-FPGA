#pragma once

#include "types.hpp"

void compute_distances(
	// in initialization
	const int query_num,
	const int d,
	// in runtime (stream)
    hls::stream<ap_uint<512>>& s_query_vectors, 
	hls::stream<ap_uint<512>>& s_fetched_vectors,
	hls::stream<int>& s_finish_query_in,
	
	// out (stream)
	hls::stream<float>& s_distances,
	hls::stream<int>& s_finish_query_out
) {

    // Wenqi comments: future upgrade -> less check on query finish, this will delay the progress
    //  TOOD: add a signal: everytime we check finish, check how many more computations should the unit expect

	const int vec_AXI_num = d / FLOAT_PER_AXI; 

    float query_vector[D_MAX];
#pragma HLS unroll variable=query_vector factor=float_per_axi

	bool first_iter_s_query_vectors = true;

    for (int qid = 0; qid < query_num; qid++) {

        // read query vector
		if (first_iter_s_query_vectors) {
			while (s_query_vectors.empty()) {}
			first_iter_s_query_vectors = false;
		}
        for (int i = 0; i < vec_AXI_num; i++) {
		#pragma HLS pipeline II=1
			ap_uint<512> query_reg = s_query_vectors.read();
			ap_uint<32> query_reg_uint32_array[FLOAT_PER_AXI];
			#pragma HLS array_partition variable=query_reg_uint32_array complete
			ap_uint<32> query_reg_float_array[FLOAT_PER_AXI];
			#pragma HLS array_partition variable=query_reg_float_array complete
			for (int j = 0; j < FLOAT_PER_AXI; j++) {
			#pragma HLS unroll
				query_reg_uint32_array[j] = query_reg.range(32 * (j + 1) - 1, 32 * j);
				query_reg_float_array[j] = *((float*) (&query_reg_uint32_array[j]));
				query_vector[i * FLOAT_PER_AXI + j] = query_reg_float_array[j];
			}
        }

        while (true) {
            
            if (!s_finish_query_in.empty() && s_fetched_vectors.empty()) {
				s_finish_query_out.write(s_finish_query_in.read());
				break;
            } else if (!s_fetched_vectors.empty()) {

				float distance = 0;

                for (int i = 0; i < vec_AXI_num; i++) {
                #pragma HLS pipeline II=1

					// read dist vector
					ap_uint<512> db_vec_reg = s_fetched_vectors.read();
					ap_uint<32> db_vec_reg_uint32_array[FLOAT_PER_AXI];
					#pragma HLS array_partition variable=db_vec_reg_uint32_array complete
                    float database_vector_partial[FLOAT_PER_AXI];
					#pragma HLS array_partition variable=database_vector_partial complete
                    for int (int s = 0; s < FLOAT_PER_AXI; s++) {
                    #pragma HLS unroll
						db_vec_reg_uint32_array[s] = db_vec_reg.range(32 * (s + 1) - 1, 32 * s);
						database_vector_partial[s] = *((float*) (&db_vec_reg_uint32_array[s]));
                    }

                    // compute L2 distance
                    float distance_partial = 0;
                    for (int s = 0; s < FLOAT_PER_AXI; s++) {
                    #pragma HLS unroll
                        float diff = query_vector[i * FLOAT_PER_AXI + s] - database_vector_partial[s];
                        distance_partial += diff * diff;
                    }

					// accumulate distance
					distance += distance_partial;
                }

				// write results
				s_distances.write(distance);
            }
        }
    }
}

