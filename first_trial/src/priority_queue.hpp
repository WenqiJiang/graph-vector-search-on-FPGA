#pragma once 

#include "constants.hpp"
#include "types.hpp"

template<typename T, const int hardware_queue_size, Order order> 
class Priority_queue;

template<const int hardware_queue_size> 
class Priority_queue<result_t, hardware_queue_size, Collect_smallest> {

    public: 

        result_t* queue;
		int runtime_queue_size;

        Priority_queue(const int runtime_queue_size) {
#pragma HLS inline

        	result_t queue_array[hardware_queue_size];
#pragma HLS array_partition variable=queue_array complete
			this->queue = queue_array;
			this->runtime_queue_size = runtime_queue_size; // must <= hardware_queue_size
			reset_queue(runtime_queue_size);
        }

		// an example of insertion and ejection
        void insert_wrapper(
            const int query_num,
            hls::stream<int> &s_control_iter_num_per_query,
            hls::stream<result_t> &s_input, 
            hls::stream<result_t> &s_output) {

			const int sort_swap_round = this->runtime_queue_size % 2 == 0? 
				this->runtime_queue_size / 2 : this->runtime_queue_size / 2 + 1;

            for (int query_id = 0; query_id < query_num; query_id++) {

				reset_queue(this->queue, runtime_queue_size);
                int iter_num = s_control_iter_num_per_query.read();

                // insert & sort
                for (int i = 0; i < iter_num + sort_swap_round; i++) {
#pragma HLS pipeline II=1
					if (i < iter_num) {
						result_t reg = s_input.read();
						this->queue[0] = this->queue[0].dist < reg.dist? this->queue[0] : reg;
					}
                    compare_swap_array_step_A();
                    compare_swap_array_step_B();
                }

                // write
                for (int i = 0; i < runtime_queue_size; i++) {
#pragma HLS pipeline II=1
                    s_output.write(this->queue[i]);
                }
            }
        }


    // private:
    
        void compare_swap(int idxA, int idxB) {
            // if smaller -> swap to right
            // note: idxA must < idxB
#pragma HLS inline
            if (this->queue[idxA].dist < this->queue[idxB].dist) {
                result_t regA = this->queue[idxA];
                result_t regB = this->queue[idxB];
                this->queue[idxA] = regB;
                this->queue[idxB] = regA;
            }
        }

        void compare_swap_array_step_A() {
            // start from idx 0, odd-even swap
#pragma HLS inline
            for (int j = 0; j < hardware_queue_size / 2; j++) {
#pragma HLS UNROLL
                compare_swap(2 * j, 2 * j + 1);
            }
        }
                    
        void compare_swap_array_step_B() {
            // start from idx 1, odd-even swap
#pragma HLS inline
            for (int j = 0; j < (hardware_queue_size - 1) / 2; j++) {
#pragma HLS UNROLL
                compare_swap(2 * j + 1, 2 * j + 2);
            }
        }

		void reset_queue(const int runtime_queue_size) {
#pragma HLS inline
			// 0 ~ runtime_queue_size - 1: valid
			for (int i = 0; i < runtime_queue_size; i++) {
#pragma HLS UNROLL
				this->queue[i].node_id = -1;
				this->queue[i].level_id = -1;
				this->queue[i].dist = large_float;
			}
			// runtime_queue_size ~ hardware_queue_size - 1: invalid range
			for (int i = runtime_queue_size; i < hardware_queue_size; i++) {
#pragma HLS UNROLL
				this->queue[i].node_id = -1;
				this->queue[i].level_id = -1;
				this->queue[i].dist = -large_float;
			}
		}
};
