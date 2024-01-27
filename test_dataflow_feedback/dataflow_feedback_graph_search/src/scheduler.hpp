#pragma once

#include "types.hpp"

void task_scheduler(
	const int query_num, 
	const int max_link_num_top, 
	const int max_link_num_base,
	// in runtime (should from DRAM)
	float* query_vectors,
	// out streams
	hls::stream<int>& s_finish_query_task_scheduler
) {
	
}