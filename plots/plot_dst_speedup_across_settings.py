"""
This script shows the speedup brought by DST, given different settings (mostly degrees and dataset dimensionality)

Example usage:
    # plot all (hard-coded) settings: inter-query
    python plot_dst_speedup_across_settings.py --df_path ../perf_test_scripts/saved_df/throughput_FPGA_inter_query_4_chan.pickle --suffix inter_query
    # plot all (hard-coded) settings: intra-query
    python plot_dst_speedup_across_settings.py --df_path ../perf_test_scripts/saved_df/throughput_FPGA_intra_query_4_chan.pickle --suffix intra_query
"""

import argparse
import matplotlib
import matplotlib.pyplot as plt
import seaborn
import numpy as np
import pandas as pd

# plt.style.use('seaborn')
plt.style.use('seaborn-colorblind')

parser = argparse.ArgumentParser()

parser.add_argument('--df_path', type=str, default="perf_df.pickle", help="the performance pickle file to save the dataframe")
parser.add_argument('--suffix', type=str, default="inter_query", help="suffix of the output")

args = parser.parse_args()


def get_max_speedup(df, graph_type, dataset, max_degree, ef, print_msg=True):
    # select rows 
    df = df.loc[(df['graph_type'] == graph_type) & (df['dataset'] == dataset) & (df['max_degree'] == max_degree) & (df['ef'] == ef)]

    # baseline: max_mg=1, max_mc=1
    row = df.loc[(df['max_cand_per_group'] == 1) & (df['max_group_num_in_pipe'] == 1)]
    assert len(row) == 1
    t_baseline = row['time_ms_kernel'].values[0]

    # get the min time across all settings, get idx
    idx_min = df['time_ms_kernel'].idxmin()
    if print_msg:
        print("Best performance: mc: {}\tmg: {}".format(df.loc[idx_min]['max_cand_per_group'], df.loc[idx_min]['max_group_num_in_pipe']))
    t_min = df.loc[idx_min]['time_ms_kernel']
    speedup = t_baseline / t_min

    return speedup

def get_recall_improvement(df, graph_type, dataset, max_degree, ef, k=10):
    # select rows 
    df = df.loc[(df['graph_type'] == graph_type) & (df['dataset'] == dataset) & (df['max_degree'] == max_degree) & (df['ef'] == ef)]

    # baseline: max_mg=1, max_mc=1
    row = df.loc[(df['max_cand_per_group'] == 1) & (df['max_group_num_in_pipe'] == 1)]
    assert len(row) == 1
    t_baseline = row['time_ms_kernel'].values[0]
    baseline_recall = row['recall_{}'.format(k)].values[0] * 100

    # get the min time across all settings, get idx
    idx_min = df['time_ms_kernel'].idxmin()
    recall = df.loc[idx_min]['recall_{}'.format(k)] * 100
    recall_improvement = recall - baseline_recall 

    return recall_improvement

def plot_speedup(df, graph_type="HNSW", datasets=['SIFT10M', 'Deep10M'], max_degrees=[16, 32, 64], ef=64, suffix='inter_query'):

    # get three subplots, horizontally 
    fig, ax = plt.subplots(1, 1, figsize=(4, 1.2))

    label_font = 12
    markersize = 10
    tick_font = 12

    markers = ['o', '^', 'x', '+', 's', 'D']
    plots = []
    for i, dataset in enumerate(datasets):
        x_labels = [str(md) for md in max_degrees]
        speedup_per_md = [get_max_speedup(df, graph_type, dataset, md, ef, print_msg=False) for md in max_degrees]
        plot = ax.plot(x_labels, speedup_per_md, marker=markers[i], markersize=markersize) #color=color_plot0, 
        plots.append(plot)

    ax.legend([plot[0] for plot in plots], datasets, loc='upper center', fontsize=label_font, ncol=2, frameon=False)
    ax.tick_params(top=False, bottom=True, left=True, right=False, labelleft=True, labelsize=tick_font)
    ax.get_xaxis().set_visible(True)
    ax.set_xlabel('max degree per node', fontsize=label_font)
    ax.set_ylabel('DST speedup\nover BFS', fontsize=label_font)
    ax.set_ylim(1, 4)

    # # doted line y = 1, with annotation: BFS
    # ax.axhline(y=1, color='black', linestyle='--', linewidth=0.5)
    # ax.text(0.0, 1.05, 'Baseline: BFS', fontsize=label_font)
    
    # title with graph name
    if suffix == 'inter_query':
        ax.set_title("Inter-query parallel, Graph: " + graph_type, fontsize=label_font)
    elif suffix == 'intra_query':
        ax.set_title("Intra-query parallel, Graph: " + graph_type, fontsize=label_font)

    # print speedup range
    print("\n=== {}, Graph: {} ===".format(suffix, graph_type))
    min_speedup = 100000
    max_speedup = 0
    for dataset in datasets:
        speedup_per_md = [get_max_speedup(df, graph_type, dataset, md, ef, print_msg=False) for md in max_degrees]
        print("Dataset: {}, Speedup: {:.2f}, {:.2f}, {:.2f}".format(dataset, *speedup_per_md))
        min_speedup = min(min_speedup, min(speedup_per_md))
        max_speedup = max(max_speedup, max(speedup_per_md))
    print("Speedup range: {:.2f} - {:.2f}".format(min_speedup, max_speedup))

    min_recall_improve = 100000
    max_recall_improve = 0
    for dataset in datasets:
        recall_improvements = [get_recall_improvement(df, graph_type, dataset, md, ef, k=10) for md in max_degrees]
        print("Dataset: {}, Recall@10 improvement (%): {:.2f}, {:.2f}, {:.2f}".format(dataset, *recall_improvements))
        min_recall_improve = min(min_recall_improve, min(recall_improvements))
        max_recall_improve = max(max_recall_improve, max(recall_improvements))
    print("Recall improvement (%) range: {:.2f} - {:.2f}".format(min_recall_improve, max_recall_improve))

    plt.savefig('./images/dst_speedup_across_settings/dst_speedup_across_settings_{}_{}.png'.format(graph_type, suffix), transparent=False, dpi=200, bbox_inches="tight")

    # plt.show()
 
if __name__ == "__main__":
    # load dataframe
    df = pd.read_pickle(args.df_path)
    # graph_types = ['HNSW']
    graph_types = ['HNSW', 'NSG']

    for graph_type in graph_types:
        datasets = ['SIFT10M', 'Deep10M']
        # datasets = ['SIFT1M', 'SIFT10M', 'SBERT1M']
        max_degrees=[16, 32, 64]
        ef=64
        plot_speedup(df, graph_type=graph_type, datasets=datasets, max_degrees=max_degrees, ef=ef, suffix=args.suffix)
