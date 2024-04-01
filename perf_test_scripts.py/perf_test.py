"""
Test the join performance on a single case. 

This script invoke the C++ sync traversal program first to get the built trees 
	from the selected datasets. It then calculate the tree depth, and pass the 
	parameters to the FPGA program. Finally, it gets the FPGA performance in both
	e2e and kernel execution.

Example Usage:

python perf_test.py \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_single_DDR_single_layer_v1.1_async_more_debug \
--max_cand_batch_size 3 --max_async_stage_num 6
--save_df perf_df.pickle
"""

import os
import re
import numpy as np
import argparse 
import pandas as pd

from utils import assert_keywords_in_file, get_number_file_with_keywords

parser = argparse.ArgumentParser()
parser.add_argument('--FPGA_project_dir', type=str, default='/mnt/scratch/wenqi/tmp_bitstreams/FPGA_single_DDR_single_layer_v1.1_async_more_debug')
parser.add_argument('--FPGA_host_name', type=str, default='host', help="the name of the exe of the FPGA host")
parser.add_argument('--FPGA_bin_name', type=str, default='xclbin/vadd.hw.xclbin', help="the name (as well as the subdir) of the FPGA bitstream")
parser.add_argument('--FPGA_log_name', type=str, default='summary.csv', help="the name of the FPGA perf summary file")
parser.add_argument('--max_cand_batch_size', type=int, default=3, help="max batch size per iter")
parser.add_argument('--max_async_stage_num', type=int, default=6, help="max number of stages on the fly")
parser.add_argument('--save_df', type=str, default='perf_df.pickle', help="the performance pickle file to save the dataframe")

args = parser.parse_args()
FPGA_project_dir = args.FPGA_project_dir
FPGA_host_name = args.FPGA_host_name
FPGA_bin_name = args.FPGA_bin_name
FPGA_log_name = args.FPGA_log_name
max_cand_batch_size_in = args.max_cand_batch_size
max_async_stage_num_in = args.max_async_stage_num

def get_FPGA_summary_time(fname):
	"""
	Given an FPGA summary file (summary.csv),
		return the performance number in ms for both scheduler and executor

	Key log:
	Device,Compute Unit,Kernel,Global Work Size,Local Work Size,Number Of Calls,Dataflow Execution,Max Overlapping Executions,Dataflow Acceleration,Total Time (ms),Minimum Time (ms),Average Time (ms),Maximum Time (ms),Clock Frequency (MHz),
	xilinx_u250_gen3x16_xdma_shell_4_1-0,vadd_1,vadd,1:1:1,1:1:1,1,Yes,1,1.000000x,967.647,967.647,967.647,967.647,200,
	"""
	pattern = r"(\d+.\d+)" # float
	kernel_keyword = 'xilinx_u250_gen3x16_xdma_shell_4_1-0,vadd_1,vadd,1:1:1,1:1:1,1,Yes,1,1.000000x,'
	time_ms_kernel = None
	
	with open(fname) as f:
		lines = f.readlines()
		for line in lines:
			if kernel_keyword in line:
				new_line = line.replace(kernel_keyword, "")
				time_ms_kernel = re.findall(pattern, new_line)[0]
				if '.' in time_ms_kernel:
					time_ms_kernel = float(time_ms_kernel)
				else: # only has int part, e.g., 1146,1146
					time_ms_kernel = float(re.findall(r"\d+", time_ms_kernel)[0])
	assert time_ms_kernel is not None

	return time_ms_kernel

if __name__ == '__main__':

	# generate a random log name
	log_FPGA = 'log_FPGA' + str(np.random.randint(0, 1000))

	# create a dataframe, with primary key being (max_cand_batch_size, max_async_stage_num) both int
	#   and columns being (time_ms_kernel, recall_1, recall_10) all float
	#   and debug signals including: avg_hops, avg_visited
	df = pd.DataFrame(columns=['max_cand_batch_size', 'max_async_stage_num', 
							'time_ms_kernel', 'recall_1', 'recall_10',
							'avg_hops', 'avg_visited'])

	for max_cand_batch_size in range(1, max_cand_batch_size_in + 1):
		for max_async_stage_num in range(1, max_async_stage_num_in + 1):
			"""
			std::cout << "Usage: " << argv[0] << "<1: xclbin>  <2: max_cand_batch_size> <3: max_async_stage_num> " << std::endl;
			"""
			host_full = os.path.join(FPGA_project_dir, FPGA_host_name)
			xclbin_full = os.path.join(FPGA_project_dir, FPGA_bin_name)
			cmd_FPGA = f"{host_full} {xclbin_full} {max_cand_batch_size} {max_async_stage_num} > {log_FPGA}"
			print("Executing FPGA command:\n", cmd_FPGA)
			os.system(cmd_FPGA)
			
			# assert assert_keywords_in_file(log_FPGA, "Result correct!") == True
			recall_1 = get_number_file_with_keywords(log_FPGA, "Recall@1=", "float")
			recall_10 = get_number_file_with_keywords(log_FPGA, "Recall@10=", "float")
			avg_hops = get_number_file_with_keywords(log_FPGA, "Average #hops on base layer=", "float", allow_none=True)
			avg_visited = get_number_file_with_keywords(log_FPGA, "Average #visited nodes=", "float", allow_none=True)

			# summary will be generated in the current directory
			time_ms_kernel = get_FPGA_summary_time(FPGA_log_name)
			
			print("max_cand_batch_size: {}, max_async_stage_num: {}".format(max_cand_batch_size, max_async_stage_num))
			print("FPGA end-to-end: {} ms".format(time_ms_kernel))
			print("Recall@1: {}\nRecall@10: {}".format(recall_1, recall_10))
			print("")

			df = df.append({'max_cand_batch_size': max_cand_batch_size, 'max_async_stage_num': max_async_stage_num, 
				'time_ms_kernel': time_ms_kernel, 'recall_1': recall_1, 'recall_10': recall_10, 
				'avg_hops': avg_hops, 'avg_visited': avg_visited}, ignore_index=True)
			
	# print the entire dataframe, show all the results
	print(df)
	# save 
	if args.save_df is not None:
		df.to_pickle(args.save_df, protocol=4)

	# show the row with best performance (min kernel time)
	min_time_idx = df['time_ms_kernel'].idxmin()
	print("Best performance:")
	print(df.iloc[min_time_idx])
	
	# remove log
	os.system(f"rm {log_FPGA}")
	os.system("rm *csv")