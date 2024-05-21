
import argparse
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os

# plt.style.use('ggplot')
plt.style.use('seaborn-colorblind') 

parser = argparse.ArgumentParser()
parser.add_argument('--df_path_inter_query', type=str, default="../perf_test_scripts/saved_df/latency_FPGA_inter_query_4_chan.pickle", help="the performance pickle file to save the dataframe")
args = parser.parse_args()

def get_slowest_fastest_row(df, graph_type, dataset, max_degree, ef, batch_size):
    # select rows 
    df = df.loc[(df['graph_type'] == graph_type) & (df['dataset'] == dataset) & 
                (df['max_degree'] == max_degree) & (df['ef'] == ef) & (df['batch_size'] == batch_size)]

    # baseline: max_mg=1, max_mc=1
    row_baseline = df.loc[(df['max_cand_per_group'] == 1) & (df['max_group_num_in_pipe'] == 1)]
    assert len(row_baseline) == 1
    # t_baseline = row_baseline['time_ms_kernel'].values[0]

    # get the row with min and max time
    row_fastest = df.loc[df['time_ms_kernel'].idxmin()]
    # row_slowest = df.loc[df['time_ms_kernel'].idxmax()]
    # print("Slowest:", row_slowest)
    # print("Fastest:", row_fastest)
    # assert row_slowest['time_ms_kernel'] == t_baseline
    
    return row_baseline, row_fastest

def get_best_qps(df, graph_type, dataset, max_degree, ef, batch_size=10000):
    row_slowest, row_fastest = get_slowest_fastest_row(df, graph_type, dataset, max_degree, ef, batch_size)
    best_qps = 10000 * 1000 / row_fastest['time_ms_kernel']
    return best_qps




def plot_throughput(datasets=["SIFT10M", "Deep10M"], graph_types=["HNSW", "NSG"], max_degree=64, ef=64):
    x_labels = []
    y_cpu = dict()
    y_fpga = dict()
    for dataset in datasets:
        for graph_type in graph_types:
            label = f"{dataset}-{graph_type}"
            x_labels.append(label)

            batch_size = 10000

            """ CPU Part """
            folder_name_CPU = './saved_perf_CPU'
            if graph_type == "HNSW":
                f_name_CPU = os.path.join(folder_name_CPU, "r630_hnsw.pickle")
                df_cpu = pd.read_pickle(f_name_CPU)
                df_selected = df_cpu[(df_cpu['dataset'] == dataset) & (df_cpu['max_degree'] == max_degree) & (df_cpu['ef'] == ef) & (df_cpu['batch_size'] == batch_size)]
                assert len(df_selected) == 1
            elif graph_type == "NSG":
                f_name_CPU = os.path.join(folder_name_CPU, "r630_nsg.pickle")
                df_cpu = pd.read_pickle(f_name_CPU)
                df_selected = df_cpu[(df_cpu['dataset'] == dataset) & (df_cpu['max_degree'] == max_degree) & (df_cpu['search_L'] == ef) & (df_cpu['batch_size'] == batch_size)]
                print(df_selected)
                assert len(df_selected) == 1
            # get CPU qps
            qps_cpu = df_selected['qps'].values[0]
            y_cpu[label] = qps_cpu


            """ FPGA Part """
            df_fpga = pd.read_pickle(args.df_path_inter_query)
            qps_fpga = get_best_qps(df_fpga, graph_type, dataset, max_degree, ef, batch_size)
            y_fpga[label] = qps_fpga

    def get_error_bar(d):
        """
        Given the key, return a dictionary of std deviation
        """
        dict_error_bar = dict()
        for key in d:
            array = d[key]
            dict_error_bar[key] = np.std(array)
        return dict_error_bar

    def get_mean(d):
        """
        Given the key, return a dictionary of mean
        """
        dict_mean = dict()
        for key in d:
            array = d[key]
            dict_mean[key] = np.average(array)
        return dict_mean

    def get_y_array(d, keys):
        """
        Given a dictionary, and a selection of keys, return an array of y value
        """
        y = []
        for key in keys:
            y.append(d[key])
        return y


    y_cpu_means = get_y_array(get_mean(y_cpu), x_labels)
    y_fpga_means = get_y_array(get_mean(y_fpga), x_labels)

    y_cpu_error_bar = get_y_array(get_error_bar(y_cpu), x_labels)
    y_fpga_error_bar = get_y_array(get_error_bar(y_fpga), x_labels)

    x = np.arange(len(x_labels))  # the label locations
    width = 0.35  # the width of the bars

    fig, ax = plt.subplots(figsize=(6, 3))
    rects1  = ax.bar(x - width/2, y_cpu_means, width)#, label='Men')
    rects2 = ax.bar(x + width/2, y_fpga_means, width)#, label='Women')
    ax.errorbar(x - width / 2, y_cpu_means, yerr=y_cpu_error_bar, fmt=',', ecolor='black', capsize=1.5)
    ax.errorbar(x + width / 2, y_fpga_means, yerr=y_fpga_error_bar, fmt=',', ecolor='black', capsize=1.5)


    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel('Queries per second (QPS)')
    # ax.set_title('Throughput comparison between CPU and FPGA')
    ax.set_xticks(x)
    ax.set_xticklabels(x_labels)
    ax.legend([rects1, rects2], ["CPU", "FPGA"], loc=(0, 1.02), ncol=2, \
        facecolor='white', framealpha=1, frameon=False)


    def autolabel(rects):
        """Attach a text label above each bar in *rects*, displaying its height."""
        for rect in rects:
            height = rect.get_height()
            ax.annotate('{:.2f}'.format(height),
                        xy=(rect.get_x() + rect.get_width() / 2, height),
                        xytext=(0, 3),  # 3 points vertical offset
                        textcoords="offset points",
                        ha='center', va='bottom')


    autolabel(rects1)
    autolabel(rects2)

    # plt.rcParams.update({'figure.autolayout': True})

    plt.savefig('./images/throughput_CPU_FPGA.png', transparent=False, dpi=200, bbox_inches="tight")
    # plt.show()

if __name__ == "__main__":
    datasets=["SIFT10M", "Deep10M"]
    # graph_types=["HNSW"]
    graph_types=["HNSW", "NSG"]
    max_degree=64
    ef=64

    plot_throughput(datasets=datasets, graph_types=graph_types, max_degree=max_degree, ef=ef)