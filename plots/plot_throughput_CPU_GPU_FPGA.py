
import argparse
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os

# plt.style.use('ggplot')
plt.style.use('seaborn-pastel') 
# plt.style.use('seaborn-colorblind') 

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
    y_gpu = dict()
    y_fpga = dict()
    recall_1 = []
    recall_10 = []
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
                print("CPU:", df_selected)
                assert len(df_selected) == 1
            # get CPU qps
            qps_cpu = df_selected['qps'].values[0]
            y_cpu[label] = qps_cpu
            recall_1_CPU = df_selected['recall_1'].values[0]
            recall_10_CPU = df_selected['recall_10'].values[0]
            recall_1.append(recall_1_CPU)
            recall_10.append(recall_10_CPU)

            """ GPU Part """
            folder_name_GPU = './saved_perf_GPU'
            # GPU only supports HNSW
            if graph_type == "HNSW":
                f_name_GPU = os.path.join(folder_name_GPU, "3090_ggnn.pickle")
                df_gpu = pd.read_pickle(f_name_GPU)
                degree_gpu = 32 # gpu used full knn graph with prunning, thus average degree ~= max degree
                max_iter_gpu = 400 # 400 iterations achieves recall near CPU
                df_selected = df_gpu[(df_gpu['dataset'] == dataset) & (df_gpu['KBuild'] == degree_gpu) & (df_gpu['MaxIter'] == max_iter_gpu) & (df_gpu['batch_size'] == batch_size)]
                print("GPU:", df_selected)
                assert len(df_selected) == 1
                qps_gpu = df_selected['qps'].values[0]
                y_gpu[label] = qps_gpu

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

    x_labels_with_recall = [x + f"\nR@10≥{recall_10[i]*100:.2f}%" for i, x in enumerate(x_labels)]
    x_labels_gpu = [x for x in x_labels if "HNSW" in x]

    bandwidth_cpu = 256.0
    bandwidth_fpga = 77.0
    bandwidth_gpu = 935.8 

    # the qps column is divided by bandwidth
    def normalize_qps(d, bandwidth):
        d_return = dict()
        for k, v in d.items():
            # if v is list
            if isinstance(v, list):
                d_return[k] = [val / bandwidth for val in v]
            else:
                d_return[k] = v / bandwidth
        return d_return
    
    y_cpu_norm = normalize_qps(y_cpu, bandwidth_cpu)
    y_gpu_norm = normalize_qps(y_gpu, bandwidth_gpu)
    y_fpga_norm = normalize_qps(y_fpga, bandwidth_fpga)

    y_cpu_means = get_y_array(get_mean(y_cpu), x_labels)
    y_gpu_means = get_y_array(get_mean(y_gpu), x_labels_gpu)
    y_fpga_means = get_y_array(get_mean(y_fpga), x_labels)

    # print("CPU throughput:", y_cpu_means)
    # print("GPU throughput:", y_gpu_means)
    # print("FPGA throughput:", y_fpga_means)
    speedup_over_cpu = np.array(y_fpga_means) / np.array(y_cpu_means)
    y_fpga_means_hnsw = [y_fpga_means[i] for i, x in enumerate(x_labels) if "HNSW" in x]
    speedup_over_gpu = np.array(y_fpga_means_hnsw) / np.array(y_gpu_means)
    print("FPGA throughput speedup over CPU: {:.2f}~{:.2f}".format(np.min(speedup_over_cpu), np.max(speedup_over_cpu)))
    print("FPGA throughput speedup over GPU: {:.2f}~{:.2f}".format(np.min(speedup_over_gpu), np.max(speedup_over_gpu)))

    y_cpu_means_norm = get_y_array(get_mean(y_cpu_norm), x_labels)
    y_gpu_means_norm = get_y_array(get_mean(y_gpu_norm), x_labels_gpu)
    y_fpga_means_norm = get_y_array(get_mean(y_fpga_norm), x_labels)
    
    # print("Normalized CPU throughput:", y_cpu_means_norm)
    # print("Normalized GPU throughput:", y_gpu_means_norm)
    # print("Normalized FPGA throughput:", y_fpga_means_norm)
    speedup_over_cpu_norm = np.array(y_fpga_means_norm) / np.array(y_cpu_means_norm)
    y_fpga_means_norm_hnsw = [y_fpga_means_norm[i] for i, x in enumerate(x_labels) if "HNSW" in x]
    speedup_over_gpu_norm = np.array(y_fpga_means_norm_hnsw) / np.array(y_gpu_means_norm)
    print("Normalized FPGA throughput speedup over CPU: {:.2f}~{:.2f}".format(np.min(speedup_over_cpu_norm), np.max(speedup_over_cpu_norm)))
    print("Normalized FPGA throughput speedup over GPU: {:.2f}~{:.2f}".format(np.min(speedup_over_gpu_norm), np.max(speedup_over_gpu_norm)))

    y_cpu_error_bar = get_y_array(get_error_bar(y_cpu), x_labels)
    y_gpu_error_bar = get_y_array(get_error_bar(y_gpu), x_labels_gpu)
    y_fpga_error_bar = get_y_array(get_error_bar(y_fpga), x_labels)

    y_cpu_error_bar_norm = get_y_array(get_error_bar(y_cpu_norm), x_labels)
    y_gpu_error_bar_norm = get_y_array(get_error_bar(y_gpu_norm), x_labels_gpu)
    y_fpga_error_bar_norm = get_y_array(get_error_bar(y_fpga_norm), x_labels)

    x = np.arange(len(x_labels))  # the label locations
    x_gpu = np.array([x_labels.index(x) for x in x_labels_gpu if "HNSW" in x])
    width = 0.2  # the width of the bars

    # two subplots (top and down), share the x axis
    fig, (ax, ax_norm) = plt.subplots(2, 1, figsize=(8, 3))
    fig.subplots_adjust(hspace=0.1)
    ax_norm.get_shared_x_axes().join(ax, ax_norm)

    label_font = 10
    tick_font = 11
    legend_font = 11
    title_font = 12
    error_bar_cap_size = 5

    # hatch styles: https://matplotlib.org/3.4.3/gallery/shapes_and_collections/hatch_style_reference.html
    hatch_fpga = '---' 
    hatch_gpu = '////'
    rects1  = ax.bar(x - width, y_cpu_means, width)#, hatch='//')
    rects2 = ax.bar(x, y_fpga_means, width, hatch=hatch_fpga)
    rects3 = ax.bar(x_gpu + width, y_gpu_means, width, hatch=hatch_gpu)
    ax.errorbar(x - width, y_cpu_means, yerr=y_cpu_error_bar, fmt=',', ecolor='black', capsize=error_bar_cap_size)
    ax.errorbar(x , y_fpga_means, yerr=y_fpga_error_bar, fmt=',', ecolor='black', capsize=error_bar_cap_size)
    ax.errorbar(x_gpu + width, y_gpu_means, yerr=y_gpu_error_bar, fmt=',', ecolor='black', capsize=error_bar_cap_size)

    rects1_norm  = ax_norm.bar(x - width, y_cpu_means_norm, width)
    rects2_norm = ax_norm.bar(x, y_fpga_means_norm, width, hatch=hatch_fpga)
    rects3_norm = ax_norm.bar(x_gpu + width, y_gpu_means_norm, width, hatch=hatch_gpu)
    ax_norm.errorbar(x - width, y_cpu_means_norm, yerr=y_cpu_error_bar_norm, fmt=',', ecolor='black', capsize=error_bar_cap_size)
    ax_norm.errorbar(x , y_fpga_means_norm, yerr=y_fpga_error_bar_norm, fmt=',', ecolor='black', capsize=error_bar_cap_size)
    ax_norm.errorbar(x_gpu + width, y_gpu_means_norm, yerr=y_gpu_error_bar_norm, fmt=',', ecolor='black', capsize=error_bar_cap_size)

    # Add some text for labels, title and custom x-axis tick labels, etc.
    # ax.set_ylabel('QPS without SLA', fontsize=label_font)
    ax.set_ylabel('Queries per sec\n(QPS) without SLA', fontsize=label_font)
    ax_norm.set_ylabel('Norm. QPS\n(QPS per GB/s)', fontsize=label_font)
    # ax_norm.set_ylabel('QPS per GB/s', fontsize=label_font)
    # ax.set_title('Throughput comparison between CPU and FPGA')
    # ax.set_xticks(x) 
    # no x ticks for ax
    ax.set_xticks([])
    ax_norm.set_xticks(x)
    ax_norm.set_xticklabels(x_labels_with_recall, rotation=0, fontsize=tick_font)
    ax.legend([rects1, rects2, rects3], ["CPU", "Falcon", "GPU"], loc="upper center", ncol=3, \
        facecolor='white', framealpha=1, frameon=True, fontsize=legend_font)

    def autolabel(rects, ax=ax):
        """Attach a text label above each bar in *rects*, displaying its height."""
        for rect in rects:
            height = rect.get_height()
            ax.annotate("{:.2E}".format(height),
                        xy=(rect.get_x() + rect.get_width() / 2, height),
                        xytext=(0, 3),  # 3 points vertical offset
                        textcoords="offset points",
                        ha='center', va='bottom', rotation=90)

    # autolabel(rects1, ax)
    # autolabel(rects2, ax)
    # autolabel(rects3, ax)

    # autolabel(rects1_norm, ax_norm)
    # autolabel(rects2_norm, ax_norm)
    # autolabel(rects3_norm, ax_norm)

    # log scale
    ax.set_yscale('log')
    ax.set_ylim(5e3, 4e6)

    # plt.rcParams.update({'figure.autolayout': True})

    plt.savefig('./images/throughput_CPU_GPU_FPGA.png', transparent=False, dpi=200, bbox_inches="tight")
    # plt.show()

if __name__ == "__main__":
    datasets=["SIFT10M", "Deep10M"]
    # graph_types=["HNSW"]
    graph_types=["HNSW", "NSG"]
    max_degree=64
    ef=64

    plot_throughput(datasets=datasets, graph_types=graph_types, max_degree=max_degree, ef=ef)