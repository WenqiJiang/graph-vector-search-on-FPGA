# Performance Measurement & Visualization of Local FPGAs


## Throughput measurement

Below are the commands measuring the FPGA performance on SIFT / SBERT datasets

```
# SIFT1M
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT1M --max_degree 64 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT1M --max_degree 32 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT1M --max_degree 16 --min_ef 64 --max_ef 64

# SIFT10M
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT10M --max_degree 64 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT10M --max_degree 32 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 --graph_type HNSW --dataset SIFT10M --max_degree 16 --min_ef 64 --max_ef 64

# Deep1M
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_96 --graph_type HNSW --dataset Deep1M --max_degree 64 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_96 --graph_type HNSW --dataset Deep1M --max_degree 32 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_96 --graph_type HNSW --dataset Deep1M --max_degree 16 --min_ef 64 --max_ef 64

# Deep10M 
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_96 --graph_type HNSW --dataset Deep10M --max_degree 64 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_96 --graph_type HNSW --dataset Deep10M --max_degree 32 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_96 --graph_type HNSW --dataset Deep10M --max_degree 16 --min_ef 64 --max_ef 64

# SBERT 1M
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_384 --graph_type HNSW --dataset SBERT1M --max_degree 64 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_384 --graph_type HNSW --dataset SBERT1M --max_degree 32 --min_ef 64 --max_ef 64
python perf_test_throughput.py --max_cand_per_group 4 --max_group_num_in_pipe 6 --save_df saved_df/perf_FPGA_inter_query_4_chan.pickle --FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_384 --graph_type HNSW --dataset SBERT1M --max_degree 16 --min_ef 64 --max_ef 64


```

## Latency measurement


### Show cross-over between intra-query and inter-query parallelism

Evaluate various batch sizes, and the per-batch latency across datasets. Using 4 channels for both Intra-query and Inter-query parallelism.

```
# SIFT1M, Inter-query, 4 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_inter_query_4_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 \
--graph_type HNSW --dataset SIFT1M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16

# SIFT10M, Inter-query, 4 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_inter_query_4_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_128 \
--graph_type HNSW --dataset SIFT10M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16

# SBERT1M, Inter-query, 4 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_inter_query_4_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_inter_query_v1.3_4_chan_D_384 \
--graph_type HNSW --dataset SBERT1M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16


# SIFT1M, Intra-query, 4 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_intra_query_4_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_intra_query_v1.5_4_chan_D_128 \
--graph_type HNSW --dataset SIFT1M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16

# SIFT10M, Intra-query, 4 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_intra_query_4_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_intra_query_v1.5_4_chan_D_128 \
--graph_type HNSW --dataset SIFT10M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16

# SBERT1M, Intra-query, 4 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_intra_query_4_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_intra_query_v1.5_4_chan_D_384 \
--graph_type HNSW --dataset SBERT1M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16
```

### Show scalability over number of memory channels / PEs

Only evaluate Intra-query. Assuming batch size always equals to one, measure the latency speedup. Assuming the 4-channel version is already measured above, now we measure 2 chan / 1 chan.

```
# SIFT1M, Intra-query, 2 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_intra_query_2_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_intra_query_v1.5_2_chan_D_128 \
--graph_type HNSW --dataset SIFT1M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16

# SIFT10M, Intra-query, 2 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_intra_query_2_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_intra_query_v1.5_2_chan_D_128 \
--graph_type HNSW --dataset SIFT10M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16

# SBERT1M, Intra-query, 2 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_intra_query_2_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_intra_query_v1.5_2_chan_D_384 \
--graph_type HNSW --dataset SBERT1M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16

# SIFT1M, Intra-query, 1 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_intra_query_1_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_intra_query_v1.5_1_chan_D_128 \
--graph_type HNSW --dataset SIFT1M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16

# SIFT10M, Intra-query, 1 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_intra_query_1_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_intra_query_v1.5_1_chan_D_128 \
--graph_type HNSW --dataset SIFT10M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16

# SBERT1M, Intra-query, 1 chan
python perf_test_latency.py \
--max_cand_per_group 3 --max_group_num_in_pipe 6 --save_df saved_df/latency_FPGA_intra_query_1_chan.pickle \
--FPGA_project_dir /mnt/scratch/wenqi/tmp_bitstreams/FPGA_intra_query_v1.5_1_chan_D_384 \
--graph_type HNSW --dataset SBERT1M --max_degree 64 --ef 64 --min_batch_size 1 --max_batch_size 16
```