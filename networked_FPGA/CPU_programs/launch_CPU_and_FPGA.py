"""
This scripts executes the CPU index server / CPU coordinator / FPGA simulator programs by reading the config file and automatically generate the command line arguments.

Example (one CPU and one FPGA)):
	In terminal 1:
		python launch_CPU_and_FPGA.py --config_fname ./config/local_network_test_1_FPGA.yaml --mode CPU_client_simulator --TOPK 100 --query_num 10000 --batch_size 32 
	In terminal 2:
		python launch_CPU_and_FPGA.py --config_fname ./config/local_network_test_1_FPGA.yaml --mode FPGA --fpga_id 0 --TOPK 100 --query_num 10000 --batch_size 32 
"""

import argparse 
import json
import os
import yaml

import numpy as np

from helper import save_obj, load_obj

parser = argparse.ArgumentParser()
parser.add_argument('--config_fname', type=str, default='./config/local_network_test_1_FPGA.yaml')
parser.add_argument('--mode', type=str, help='CPU_client_simulator or CPU_coordinator or FPGA')

# if run in the CPU mode
parser.add_argument('--cpu_index_server_exe_dir', type=str, default='./CPU_client_simulator.bin', help="the CPP exe file")

# if run in the FPGA simulator mode
parser.add_argument('--fpga_simulator_exe_dir', type=str, default='./FPGA_simulator.bin', help="the FPGA simulator exe file")
parser.add_argument('--fpga_id', type=int, default=0, help="the FPGA ID (should start from 0, 1, 2, ...)")

# optional runtim input arguments
parser.add_argument('--TOPK', type=int, default=None)
parser.add_argument('--query_num', type=int, default=None)
parser.add_argument('--batch_size', type=int, default=None)

args = parser.parse_args()
config_fname = args.config_fname
mode = args.mode
if mode == 'CPU_client_simulator':
	cpu_index_server_exe_dir = args.cpu_index_server_exe_dir
elif mode == 'FPGA':
	fpga_simulator_exe_dir = args.fpga_simulator_exe_dir
	fpga_id = args.fpga_id
	
def get_board_ID(FPGA_IP_addr_list):
	"""
	Given the IP address, return the boardNum argument passed to the FPGA network stack
	#   alveo-u250-01: 10.253.74.12
	#   alveo-u250-02: 10.253.74.16
	#   alveo-u250-03: 10.253.74.20
	#   alveo-u250-04: 10.253.74.24
	#   alveo-u250-05: 10.253.74.28
	#   alveo-u250-06: 10.253.74.40
	"""
	if FPGA_IP_addr_list == '10.253.74.12':
		return '1'
	elif FPGA_IP_addr_list == '10.253.74.16':
		return '2'
	elif FPGA_IP_addr_list == '10.253.74.20':
		return '3'
	elif FPGA_IP_addr_list == '10.253.74.24':
		return '4'
	elif FPGA_IP_addr_list == '10.253.74.28':
		return '5'
	elif FPGA_IP_addr_list == '10.253.74.40':
		return '6'
	else:
		return None

# yaml inputs
num_FPGA = None
CPU_IP_addr = None
FPGA_IP_addr_list = None
C2F_port_list = None
F2C_port_list = None

D = None 
TOPK = None

query_num = None
batch_size = None
nprobe = None
nlist = None

query_window_size = None
batch_window_size = None
enable_index_scan = None
cpu_cores = None

config_dict = {}
with open(args.config_fname, "r") as f:
    config_dict.update(yaml.safe_load(f))
locals().update(config_dict)

assert int(num_FPGA) == len(FPGA_IP_addr_list)
assert int(num_FPGA) == len(C2F_port_list)
assert int(num_FPGA) == len(F2C_port_list)

# if query_window_size == 'auto':
# 	query_msg_size = 4 * D * (nprobe + 1) # as an approximation

if args.TOPK is not None:
	TOPK = args.TOPK
if args.batch_size is not None:
	batch_size = args.batch_size
if args.query_num is not None:
	query_num = args.query_num

if mode == 'CPU_client_simulator':
	"""
  std::cout << "Usage: " << argv[0] << " <1 num_FPGA> "
      "<2 ~ 2 + num_FPGA - 1 FPGA_IP_addr> " 
    "<2 + num_FPGA ~ 2 + 2 * num_FPGA - 1 C2F_port> " 
    "<2 + 2 * num_FPGA ~ 2 + 3 * num_FPGA - 1 F2C_port> "
    "<2 + 3 * num_FPGA D> <3 + 3 * num_FPGA TOPK> " 
    "<4 + 3 * num_FPGA query_num> " "<5 + 3 * num_FPGA batch_size> "
    "<6 + 3 * num_FPGA query_window_size> <7 + 3 * num_FPGA batch_window_size> " 
	"""

	# print the FPGA commands if the FPGA is available
	for i in range(int(num_FPGA)):
		FPGA_IP_addr = FPGA_IP_addr_list[i]	
		if get_board_ID(FPGA_IP_addr) is not None:
			board_ID = get_board_ID(FPGA_IP_addr)
			# std::cout << "Usage: " << argv[0] << " <XCLBIN File 1> <local_FPGA_IP 2> <RxPort (C2F) 3> <TxIP (CPU IP) 4> <TxPort (F2C) 5> <FPGA_board_ID 6" << std::endl;
			print(f'\nFPGA {i} commands: ')
			print("./host/host build_dir.hw.xilinx_u250_gen3x16_xdma_4_1_202210_1/network.xclbin "
				f" {FPGA_IP_addr} {C2F_port_list[i]} {CPU_IP_addr} {F2C_port_list[i]} {TOPK} \n\n")
		else:
			print("Unknown FPGA IP address: ", FPGA_IP_addr)

	# execute the CPU command
	cmd = ''
	# cmd += 'taskset --cpu-list 0-{} '.format(cpu_cores)
	cmd += ' {} '.format(cpu_index_server_exe_dir)
	cmd += ' {} '.format(num_FPGA)
	for i in range(int(num_FPGA)):
		cmd += ' {} '.format(FPGA_IP_addr_list[i])
	for i in range(int(num_FPGA)):
		cmd += ' {} '.format(C2F_port_list[i])
	for i in range(int(num_FPGA)):
		cmd += ' {} '.format(F2C_port_list[i])
	cmd += ' {} '.format(D)
	cmd += ' {} '.format(TOPK)
	cmd += ' {} '.format(query_num)
	cmd += ' {} '.format(batch_size)
	cmd += ' {} '.format(query_window_size)
	cmd += ' {} '.format(batch_window_size)
	# cmd += ' {} '.format(cpu_cores)
	print('Executing: ', cmd)
	os.system(cmd)

	print('Loading and copying profile...')
	latency_ms_distribution = np.fromfile('profile_latency_ms_distribution.double', dtype=np.float64).reshape(-1,)
	# deep copy latency_ms_distribution
	latency_ms_distribution_sorted = np.array(latency_ms_distribution)
	latency_ms_distribution_sorted.sort()
	latency_ms_min = latency_ms_distribution_sorted[0]
	latency_ms_max = latency_ms_distribution_sorted[-1]
	latency_ms_median = latency_ms_distribution_sorted[int(len(latency_ms_distribution_sorted) / 2)]
	QPS = np.fromfile('profile_QPS.double', dtype=np.float64).reshape(-1,)[0]

	print("Loaded profile: ")
	print("latency_ms_min: ", latency_ms_min)
	print("latency_ms_max: ", latency_ms_max)
	print("latency_ms_median: ", latency_ms_median)
	print("QPS: ", QPS)
	
	# config_dict['latency_ms_distribution'] = latency_ms_distribution
	# config_dict['latency_ms_min'] = latency_ms_min
	# config_dict['latency_ms_max'] = latency_ms_max
	# config_dict['latency_ms_median'] = latency_ms_median
	# config_dict['QPS'] = QPS

	# fname = os.path.basename(config_fname).split('.')[0] 
	# save_obj(config_dict, 'performance_pickle', fname)


elif mode == 'FPGA':
	"""
// std::cout << "Usage: 
//  " << argv[0] << " <Tx (CPU) IP_addr> <Tx F2C_port> <Rx C2F_port> " 
// 	"<TOPK/ef> <D> <query_num> " << std::endl;
	"""
	cmd = ''
	cmd += ' {} '.format(fpga_simulator_exe_dir)
	cmd += ' {} '.format(CPU_IP_addr)
	cmd += ' {} '.format(F2C_port_list[fpga_id])
	cmd += ' {} '.format(C2F_port_list[fpga_id])
	cmd += ' {} '.format(TOPK)
	cmd += ' {} '.format(D)
	cmd += ' {} '.format(query_num)
	print('Executing: ', cmd)
	os.system(cmd)