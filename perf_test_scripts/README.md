# 


## Measure FPGA throughput

Below are the commands measuring the FPGA performance on SIFT / SBERT datasets

```
# SIFT1M
python perf_test.py --max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT1M --max_degree 64 --min_ef 64 --max_ef 64
python perf_test.py --max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT1M --max_degree 32 --min_ef 64 --max_ef 64
python perf_test.py --max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT1M --max_degree 16 --min_ef 64 --max_ef 64

# SIFT10M
python perf_test.py --max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT10M --max_degree 64 --min_ef 64 --max_ef 64
python perf_test.py --max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT10M --max_degree 32 --min_ef 64 --max_ef 64
python perf_test.py --max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT10M --max_degree 16 --min_ef 64 --max_ef 64


# SBERT 1M
python perf_test.py --max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_384 --graph_type HNSW --dataset SBERT1M --max_degree 64 --min_ef 64 --max_ef 64
python perf_test.py --max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_384 --graph_type HNSW --dataset SBERT1M --max_degree 32 --min_ef 64 --max_ef 64
python perf_test.py --max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_384 --graph_type HNSW --dataset SBERT1M --max_degree 16 --min_ef 64 --max_ef 64


# Visualizing heatmap 6 x 6 using SIFT1M / 10M
python perf_test.py --max_cand_per_group 6 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT1M --max_degree 64 --min_ef 64 --max_ef 64
python perf_test.py --max_cand_per_group 6 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT10M --max_degree 64 --min_ef 64 --max_ef 64
```