"""
This script shows the speedup brought by DST, given different settings (mostly degrees and dataset dimensionality)

Example usage:

    python plot_dst_speedup_across_settings.py --df_path ./saved_df/throughput_FPGA_inter_query_4_chan.pickle
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

args = parser.parse_args()


def get_max_speedup(df, graph_type, dataset, max_degree, ef):
    # select rows 
    df = df.loc[(df['graph_type'] == graph_type) & (df['dataset'] == dataset) & (df['max_degree'] == max_degree) & (df['ef'] == ef)]

    # baseline: max_mg=1, max_mc=1
    row = df.loc[(df['max_cand_per_group'] == 1) & (df['max_group_num_in_pipe'] == 1)]
    assert len(row) == 1
    t_baseline = row['time_ms_kernel'].values[0]

    # get the min time across all settings
    t_min = df['time_ms_kernel'].min()
    speedup = t_baseline / t_min

    return speedup

def plot_speedup(df, graph_type="HNSW", datasets=["SIFT1M", "SBERT1M"], max_degrees=[16, 32, 64], ef=64):

    # get three subplots, horizontally 
    fig, ax = plt.subplots(1, 1, figsize=(6, 3))

    label_font = 12
    markersize = 10
    tick_font = 12

    markers = ['o', '^', 'x', '+', 's', 'D']
    plots = []
    for i, dataset in enumerate(datasets):
        x_labels = [str(md) for md in max_degrees]
        speedup_per_md = [get_max_speedup(df, graph_type, dataset, md, ef) for md in max_degrees]
        plot = ax.plot(x_labels, speedup_per_md, marker=markers[i], markersize=markersize) #color=color_plot0, 
        plots.append(plot)

    ax.legend([plot[0] for plot in plots], datasets, loc='lower center', fontsize=label_font, ncol=2)
    ax.tick_params(top=False, bottom=True, left=True, right=False, labelleft=True, labelsize=tick_font)
    ax.get_xaxis().set_visible(True)
    ax.set_xlabel('max degree per node', fontsize=label_font)
    ax.set_ylabel('DST speedup over BFS', fontsize=label_font)
    ax.set_ylim(0, 2.5)

    # doted line y = 1, with annotation: BFS
    ax.axhline(y=1, color='black', linestyle='--', linewidth=0.5)
    ax.text(0.0, 1.05, 'Baseline: best-first search (BFS)', fontsize=label_font)

    plt.savefig('./images/dst_speedup_across_settings_{}.png'.format(graph_type), transparent=False, dpi=200, bbox_inches="tight")

    # plt.show()
 
if __name__ == "__main__":
    # load dataframe
    df = pd.read_pickle(args.df_path)
    graph_types = ['HNSW']
    # graph_types = ['HNSW', 'NSG']

    for graph_type in graph_types:
        datasets = ['SIFT10M', 'SBERT1M']
        # datasets = ['SIFT1M', 'SIFT10M', 'SBERT1M']
        max_degrees=[16, 32, 64]
        ef=64
        plot_speedup(df, graph_type=graph_type, datasets=datasets, max_degrees=max_degrees, ef=ef)
