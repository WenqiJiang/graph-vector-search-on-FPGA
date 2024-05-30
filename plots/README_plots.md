# Plots

This folder contains all the plotting scripts as well as scripts used for printing performance information.

## Files and Folders

`saved_...` are the data saved for plotting.

`calculate_silicon_efficiency.py` is the script for printing silicon efficiency of different traversals. 

`plot_dst_speedup_across_settings.py` plots the speed of of DST over BFS, given different graphs, degrees, and datasets.

`plot_dst_speedup_nodes_recall.py` plots the heatmap of different DST configurations, reporting performance, hops per query, and recall.

`plot_intra_query_chan_scalability.py` shows the performance scalability of intra-query parallelism, given different numbers of processing pipelines.

`plot_latency_CPU_GPU_FPGA.py` shows the latency comparison between CPUs, GPUs, and FPGAs.

`plot_throughput_CPU_GPU_FPGA.py` shows the throughput comparison between CPUs, GPUs, and FPGAs.

(unsed) `plot_intra_vs_inter_performance.py` shows the performance cross-over points between intra and inter-query parallelism, without network latency. 

(unused) `plot_latency_FPGA.py` shows the latency distribution of only FPGAs.

## Commands for plotting all the figures for the paper

Outputs are stored in `images`.

DST convergence: 
```
python plot_distance_over_steps_different_traversals.py 
```

DST speedup over BFS:
```
# plot all (hard-coded) settings: inter-query
python plot_dst_speedup_across_settings.py --df_path ../perf_test_scripts/saved_df/throughput_FPGA_inter_query_4_chan.pickle --suffix inter_query
# plot all (hard-coded) settings: intra-query
python plot_dst_speedup_across_settings.py --df_path ../perf_test_scripts/saved_df/throughput_FPGA_intra_query_4_chan.pickle --suffix intra_query
```

DST heatmap (performance, #hops, recall):
```
# plot all (hard-coded) settings: intra-query
python plot_dst_speedup_nodes_recall.py --plot_all 1 --df_path ../perf_test_scripts/saved_df/throughput_FPGA_intra_query_4_chan.pickle --suffix intra_query --max_mc 4 --max_mg 7
# plot all (hard-coded) settings: inter-query
python plot_dst_speedup_nodes_recall.py --plot_all 1 --df_path ../perf_test_scripts/saved_df/throughput_FPGA_inter_query_4_chan.pickle --suffix inter_query --max_mc 4 --max_mg 7
```

Intra-query parallel scalability:
```
python plot_intra_query_chan_scalability.py \
	--df_path_1_chan ../perf_test_scripts/saved_df/latency_FPGA_intra_query_1_chan.pickle \
	--df_path_2_chan ../perf_test_scripts/saved_df/latency_FPGA_intra_query_2_chan.pickle \
	--df_path_4_chan ../perf_test_scripts/saved_df/latency_FPGA_intra_query_4_chan.pickle
```

Latency CPU, GPU, FPGA:
```
python plot_latency_CPU_GPU_FPGA.py 
```

Throughput CPU, GPU, FPGA:
```
python plot_throughput_CPU_GPU_FPGA.py 
```