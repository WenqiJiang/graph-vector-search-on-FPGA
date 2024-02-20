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
			
			for (int i = 0; i < hardware_queue_size; i++) {
#pragma HLS UNROLL
				// 0 ~ runtime_queue_size - 1: valid
				if (i < runtime_queue_size) {
					this->queue[i].node_id = -1;
					this->queue[i].level_id = -1;
					this->queue[i].dist = large_float;
				}
				// runtime_queue_size ~ hardware_queue_size - 1: invalid range
				else {
					this->queue[i].node_id = -1;
					this->queue[i].level_id = -1;
					this->queue[i].dist = -large_float;
				}
			}
		}

		// insert and then sort
        void insert_sort(
            hls::stream<int> &s_num_insertion,
            hls::stream<result_t> &s_input) {
#pragma HLS inline	

			const int sort_swap_round = this->runtime_queue_size % 2 == 0? 
				this->runtime_queue_size / 2 : (this->runtime_queue_size - 1) / 2;

			int num_insertion = s_num_insertion.read();

			// insert & sort
			for (int i = 0; i < num_insertion + sort_swap_round; i++) {
#pragma HLS pipeline II=2
				if (i < num_insertion) {
					result_t reg = s_input.read();
					this->queue[0] = this->queue[0].dist <= reg.dist? this->queue[0] : reg;
				}
				compare_swap_array_step_A();
				compare_swap_array_step_B();
			}
        }

		// pop top element
		void pop_top(
			hls::stream<cand_t> &s_top_candidates) {
#pragma HLS inline

			int smallest_element_position = this->runtime_queue_size - 1;

			// pop
			cand_t reg_cand = {this->queue[smallest_element_position].node_id, this->queue[smallest_element_position].level_id};
			s_top_candidates.write(reg_cand);

			// right shift
			for (int i = 1; i < hardware_queue_size; i++) {
#pragma HLS UNROLL
				this->queue[i] = this->queue[i - 1];
			}
			// reset left-most
			this->queue[0].node_id = -1;
			this->queue[0].level_id = -1;
			this->queue[0].dist = large_float;
			// reset pop position
			if (smallest_element_position < hardware_queue_size - 1) {
				this->queue[smallest_element_position + 1].node_id = -1;
				this->queue[smallest_element_position + 1].level_id = -1;
				this->queue[smallest_element_position + 1].dist = -large_float;
			}
		}
};
