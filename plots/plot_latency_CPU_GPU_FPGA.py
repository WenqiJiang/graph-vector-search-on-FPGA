import numpy as np
import os
import matplotlib
import matplotlib.pyplot as plt

from matplotlib.ticker import FuncFormatter
import pandas as pd
import seaborn as sns 

# Seaborn version >= 0.11.0

sns.set_theme(style="whitegrid")
# Set the palette to the "pastel" default palette:
# sns.set_palette("pastel")

global_intra_query_median_latency_speedup_over_GPU = []
global_intra_query_median_latency_speedup_over_CPU = []
global_inter_query_median_latency_speedup_over_GPU = []
global_inter_query_median_latency_speedup_over_CPU = []

global_intra_query_P95_latency_speedup_over_GPU = []
global_intra_query_P95_latency_speedup_over_CPU = []
global_inter_query_P95_latency_speedup_over_GPU = []
global_inter_query_P95_latency_speedup_over_CPU = []

def plot_latency(dataset='SIFT1M', graph_type="HNSW", max_degree=64, ef=64, batch_sizes=[1,2,4,8,16],
                 show_FPGA=True, show_CPU=True, show_GPU=True, add_CPU_network_latency=True):

    ### Note: For violin graph, a single violin's data must be in the same column
    ###   e.g., given 3 violin plots, each with 100 points, the shape of the array
    ###   should be (100, 3), where the first column is for the first violin and so forth
    # fake up some data


    # Wenqi: flatten the table to a table. It's a dictionary with the key as schema.
    #   The value of each key is an array.
    # label category data
    # xxx   xxx      xxx
    # yyy   yyy      yyy

    folder_name_FPGA_inter_query = 'saved_latency/FPGA_inter_query_v1_3_4_chan'
    folder_name_FPGA_intra_query = 'saved_latency/FPGA_intra_query_v1_5_4_chan'
    folder_name_CPU = './saved_perf_CPU'
    folder_name_GPU = './saved_perf_GPU'
    folder_name_CPU_network_latency = './saved_latency/CPU_two_servers'

    d = {}
    d['label'] = []
    d['data'] = []
    d['category'] = []

    recall_1_CPU = None
    recall_10_CPU = None

    latency_median_CPU = []
    latency_median_GPU = []
    latency_median_FPGA_inter_query = []
    latency_median_FPGA_intra_query = []
    
    latency_P95_CPU = []
    latency_P95_GPU = []
    latency_P95_FPGA_inter_query = []
    latency_P95_FPGA_intra_query = []

    for batch_size in batch_sizes:
        if show_FPGA:
            # load latency distribution (in double)
            # file name
            #   std::string out_fname = "latency_ms_per_batch_" + dataset + "_" + graph_type + 
            # 	  "_MD" + std::to_string(max_degree) + "_ef" + std::to_string(ef) + + "_batch_size" + std::to_string(batch_size) + ".double";
            f_name_FPGA_inter_query = os.path.join(folder_name_FPGA_inter_query, 
                "latency_ms_per_batch_" + dataset + "_" + graph_type + "_MD" + str(max_degree) + "_ef" + str(ef) + "_batch_size" + str(batch_size) + ".double")
            f_name_FPGA_intra_query = os.path.join(folder_name_FPGA_intra_query,
                "latency_ms_per_batch_" + dataset + "_" + graph_type + "_MD" + str(max_degree) + "_ef" + str(ef) + "_batch_size" + str(batch_size) + ".double")
            # load as np array
            latency_FPGA_inter_query = np.fromfile(f_name_FPGA_inter_query, dtype=np.float64)
            latency_FPGA_intra_query = np.fromfile(f_name_FPGA_intra_query, dtype=np.float64)

            latency_median_FPGA_inter_query.append(np.median(latency_FPGA_inter_query))
            latency_median_FPGA_intra_query.append(np.median(latency_FPGA_intra_query))
            latency_P95_FPGA_inter_query.append(np.percentile(latency_FPGA_inter_query, 95))
            latency_P95_FPGA_intra_query.append(np.percentile(latency_FPGA_intra_query, 95))
            
            for latency in latency_FPGA_inter_query:
                d['label'].append('batch_size={}'.format(batch_size))
                d['data'].append(latency)
                d['category'].append('Falcon Inter-query Parallel')
            
            for latency in latency_FPGA_intra_query:
                d['label'].append('batch_size={}'.format(batch_size))
                d['data'].append(latency)
                d['category'].append('Falcon Intra-query Parallel')
        if show_CPU or show_GPU:
            if add_CPU_network_latency:
                if dataset.startswith("SIFT"):
                    f_name_CPU_network = os.path.join(folder_name_CPU_network_latency, "latency_ms_per_batch_SIFT_batch_size" + str(batch_size) + ".double")
                elif dataset.startswith("Deep"):
                    f_name_CPU_network = os.path.join(folder_name_CPU_network_latency, "latency_ms_per_batch_Deep_batch_size" + str(batch_size) + ".double")
                latency_CPU_network = np.fromfile(f_name_CPU_network, dtype=np.float64)
                average_network_latency = np.mean(latency_CPU_network)
            else:
                average_network_latency = 0

            if show_CPU:
                if graph_type == "HNSW":
                    f_name_CPU = os.path.join(folder_name_CPU, "r630_hnsw.pickle")
                    df_cpu = pd.read_pickle(f_name_CPU)

                    # key_columns = ['dataset', 'max_degree', 'ef', 'omp_enable', 'max_cores', 'batch_size']
                    # result_columns = ['recall_1', 'recall_10', 'latency_ms_per_batch', 'qps']

                    # select rows, and assert only one row is selected
                    df_selected = df_cpu[(df_cpu['dataset'] == dataset) & (df_cpu['max_degree'] == max_degree) & (df_cpu['ef'] == ef) & (df_cpu['batch_size'] == batch_size)]
                    assert len(df_selected) == 1
                    recall_1_CPU = df_selected['recall_1'].values[0]
                    recall_10_CPU = df_selected['recall_10'].values[0]

                    # add latency to d
                    for latency in df_selected['latency_ms_per_batch'].values[0]:
                        # print(latency)
                        d['label'].append('batch_size={}'.format(batch_size))
                        d['data'].append(latency + average_network_latency)
                        d['category'].append('CPU')
                elif graph_type == "NSG":
                    f_name_CPU = os.path.join(folder_name_CPU, "r630_nsg.pickle")
                    df_cpu = pd.read_pickle(f_name_CPU)

                    # key_columns = ['dataset', 'max_degree', 'search_L', 'omp_enable', 'max_cores', 'batch_size']
                    # result_columns = ['recall_1', 'recall_10', 'latency_ms_per_batch', 'qps']

                    # select rows, and assert only one row is selected
                    df_selected = df_cpu[(df_cpu['dataset'] == dataset) & (df_cpu['max_degree'] == max_degree) & (df_cpu['search_L'] == ef) & (df_cpu['batch_size'] == batch_size)]
                    assert len(df_selected) == 1
                    recall_1_CPU = df_selected['recall_1'].values[0]
                    recall_10_CPU = df_selected['recall_10'].values[0]

                    # add latency to d
                    for latency in df_selected['latency_ms_per_batch'].values[0]:
                        # print(latency)
                        d['label'].append('batch_size={}'.format(batch_size))
                        d['data'].append(latency + average_network_latency)
                        d['category'].append('CPU')

                latency_median_CPU.append(np.median(df_selected['latency_ms_per_batch'].values[0]) + average_network_latency)
                latency_P95_CPU.append(np.percentile(df_selected['latency_ms_per_batch'].values[0], 95) + average_network_latency)
            if show_GPU:
                # GPU only supports HNSW
                if graph_type == "HNSW":
                    f_name_GPU = os.path.join(folder_name_GPU, "3090_ggnn.pickle")
                    df_gpu = pd.read_pickle(f_name_GPU)

                    # key_columns = ['dataset', 'KBuild', 'S', 'KQuery', 'MaxIter', 'batch_size', 'tau_query']
                    # result_columns = ['recall_1', 'recall_10', 'latency_ms_per_batch', 'qps']

                    # select rows, and assert only one row is selected
                    degree_gpu = 32 # gpu used full knn graph with prunning, thus average degree ~= max degree
                    max_iter_gpu = 400 # 400 iterations achieves recall near CPU
                    df_selected = df_gpu[(df_gpu['dataset'] == dataset) & (df_gpu['KBuild'] == degree_gpu) & (df_gpu['MaxIter'] == max_iter_gpu) & (df_gpu['batch_size'] == batch_size)]
                    assert len(df_selected) == 1

                    latency_median_GPU.append(np.median(df_selected['latency_ms_per_batch'].values[0]) + average_network_latency)
                    latency_P95_GPU.append(np.percentile(df_selected['latency_ms_per_batch'].values[0], 95) + average_network_latency)	
                    # add latency to d
                    for latency in df_selected['latency_ms_per_batch'].values[0]:
                        # print(latency)
                        d['label'].append('batch_size={}'.format(batch_size))
                        d['data'].append(latency + average_network_latency)
                        d['category'].append('GPU')

    df = pd.DataFrame(data=d)
    # print(df.index)
    # print(df.columns)

    print(f"==== {dataset}, {graph_type} ====")
    # print(f"Median Latency (ms):")
    # print(f"CPU: {latency_median_CPU}")
    # if graph_type == "HNSW":
    #     print(f"GPU: {latency_median_GPU}")
    # print(f"FPGA Inter-query Parallel: {latency_median_FPGA_inter_query}")
    # print(f"FPGA Intra-query Parallel: {latency_median_FPGA_intra_query}")

    # print(f"P95 Latency (ms):")
    # print(f"CPU: {latency_P95_CPU}")
    # if graph_type == "HNSW":
    #     print(f"GPU: {latency_P95_GPU}")
    # print(f"FPGA Inter-query Parallel: {latency_P95_FPGA_inter_query}")
    # print(f"FPGA Intra-query Parallel: {latency_P95_FPGA_intra_query}")

    inter_query_median_latency_speedup_over_CPU = np.array(latency_median_CPU) / np.array(latency_median_FPGA_inter_query)
    intra_query_median_latency_speedup_over_CPU = np.array(latency_median_CPU) / np.array(latency_median_FPGA_intra_query)
    inter_query_P95_latency_speedup_over_CPU = np.array(latency_P95_CPU) / np.array(latency_P95_FPGA_inter_query)
    intra_query_P95_latency_speedup_over_CPU = np.array(latency_P95_CPU) / np.array(latency_P95_FPGA_intra_query)
    global_intra_query_median_latency_speedup_over_CPU.extend(intra_query_median_latency_speedup_over_CPU)
    global_inter_query_median_latency_speedup_over_CPU.extend(inter_query_median_latency_speedup_over_CPU)
    global_intra_query_P95_latency_speedup_over_CPU.extend(intra_query_P95_latency_speedup_over_CPU)
    global_inter_query_P95_latency_speedup_over_CPU.extend(inter_query_P95_latency_speedup_over_CPU)
    if graph_type == "HNSW":
        inter_query_median_latency_speedup_over_GPU = np.array(latency_median_GPU) / np.array(latency_median_FPGA_inter_query)
        intra_query_median_latency_speedup_over_GPU = np.array(latency_median_GPU) / np.array(latency_median_FPGA_intra_query)
        inter_query_P95_latency_speedup_over_GPU = np.array(latency_P95_GPU) / np.array(latency_P95_FPGA_inter_query)
        intra_query_P95_latency_speedup_over_GPU = np.array(latency_P95_GPU) / np.array(latency_P95_FPGA_intra_query)
        global_intra_query_median_latency_speedup_over_GPU.extend(intra_query_median_latency_speedup_over_GPU)
        global_inter_query_median_latency_speedup_over_GPU.extend(inter_query_median_latency_speedup_over_GPU)
        global_intra_query_P95_latency_speedup_over_GPU.extend(intra_query_P95_latency_speedup_over_GPU)
        global_inter_query_P95_latency_speedup_over_GPU.extend(inter_query_P95_latency_speedup_over_GPU)
    print("Inter-query speedup over CPU (median): {:.2f}~{:.2f}x".format(np.min(inter_query_median_latency_speedup_over_CPU), np.max(inter_query_median_latency_speedup_over_CPU)))
    print(inter_query_median_latency_speedup_over_CPU)
    print("Intra-query speedup over CPU (median): {:.2f}~{:.2f}x".format(np.min(intra_query_median_latency_speedup_over_CPU), np.max(intra_query_median_latency_speedup_over_CPU)))
    print(intra_query_median_latency_speedup_over_CPU)
    print("Inter-query speedup over CPU (P95): {:.2f}~{:.2f}x".format(np.min(inter_query_P95_latency_speedup_over_CPU), np.max(inter_query_P95_latency_speedup_over_CPU)))
    # print(inter_query_P95_latency_speedup_over_CPU)
    print("Intra-query speedup over CPU (P95): {:.2f}~{:.2f}x".format(np.min(intra_query_P95_latency_speedup_over_CPU), np.max(intra_query_P95_latency_speedup_over_CPU)))
    # print(intra_query_P95_latency_speedup_over_CPU)

    if graph_type == "HNSW":
        print("Inter-query speedup over GPU (median): {:.2f}~{:.2f}x".format(np.min(inter_query_median_latency_speedup_over_GPU), np.max(inter_query_median_latency_speedup_over_GPU)))
        print(inter_query_median_latency_speedup_over_GPU)
        print("Intra-query speedup over GPU (median): {:.2f}~{:.2f}x".format(np.min(intra_query_median_latency_speedup_over_GPU), np.max(intra_query_median_latency_speedup_over_GPU)))
        print(intra_query_median_latency_speedup_over_GPU)
        print("Inter-query speedup over GPU (P95): {:.2f}~{:.2f}x".format(np.min(inter_query_P95_latency_speedup_over_GPU), np.max(inter_query_P95_latency_speedup_over_GPU)))
        print("Intra-query speedup over GPU (P95): {:.2f}~{:.2f}x".format(np.min(intra_query_P95_latency_speedup_over_GPU), np.max(intra_query_P95_latency_speedup_over_GPU)))


    plt.figure(figsize=(5.5, 2.5))
    # API: https://seaborn.pydata.org/generated/seaborn.violinplot.html
    # inner{“box”, “quartile”, “point”, “stick”, None}, optional
    # ax = sns.violinplot(data=df, scale='area', inner='box', x="label", y="data", hue="category")
    # use box plot
    ax = sns.boxplot(data=df, x="label", y="data", hue="category", showfliers=False)

    label_font = 13
    tick_font = 12
    legend_font = 12
    title_font = 13
    
    x_tick_labels = ["{}".format(i) for i in batch_sizes]
    ax.set_xticklabels(x_tick_labels)
    # ax.set_yticklabels(y_tick_labels)
    # plt.yticks(rotation=0)
    # # ax.ticklabel_format(axis='both', style='sci')
    # # ax.set_yscale("log")
    # ax.legend(loc='best', ncol=2, fontsize=legend_font)
    ax.legend(loc='upper center', ncol=2, fontsize=legend_font, frameon=False)

    ax.tick_params(length=0, top=False, bottom=True, left=True, right=False, 
        labelleft=True, labelbottom=True, labelsize=tick_font)
    ax.set_xlabel('Batch sizes', fontsize=12, labelpad=0)
    ax.set_ylabel('Latency (ms)', fontsize=label_font, labelpad=-5)
    title = f'Dataset: {dataset}, Graph: {graph_type}'
    # if recall_1_CPU is not None:
    #     title += f', R@1≥{recall_1_CPU*100:.2f}%'
    if recall_10_CPU is not None:
        title += f', R@10≥{recall_10_CPU*100:.2f}%'

    ax.set_title(title, fontsize=title_font)
    # plt.text(2, len(y_tick_labels) + 2, "Linear Heatmap", fontsize=16)

    # set y log scale
    ax.set_yscale("log")
    # set y lim
    ax.set_ylim([0.08, 80])
    
    # if dataset is SIFT and graph is HNSW, annotate the batch size of 1 with Inter/intra-query parallel
    if dataset.startswith("SIFT") and graph_type == "HNSW":
        width=0.2
        ax.annotate('Inter-q', xy=(-1.5 * width, 0.3), xytext=(-1.5 * width, 0.8),
            arrowprops=dict(facecolor='black', shrink=0., width=3, headwidth=7, frac=0.1), fontsize=12, rotation=90, verticalalignment='bottom', horizontalalignment='center')
        ax.annotate('Intra-q', xy=(-.5 * width, 0.2), xytext=(-.5 * width, 0.5),
            arrowprops=dict(facecolor='black', shrink=0., width=3, headwidth=7, frac=0.1), fontsize=12, rotation=90, verticalalignment='bottom', horizontalalignment='center')

    plt.savefig(f'./images/latency_CPU_GPU_FPGA/latency_CPU_GPU_FPGA_{dataset}_{graph_type}.png', transparent=False, dpi=200, bbox_inches="tight")

    # plt.show()

if __name__ == "__main__":
    # datasets = ["SIFT1M"]
    datasets = ["SIFT10M", "Deep10M"]
    graph_types = ["HNSW", "NSG"]
    # graph_types = ["HNSW"]

    for dataset in datasets:
        for graph_type in graph_types:
            plot_latency(dataset, graph_type, show_FPGA=True, show_CPU=True, show_GPU=True, add_CPU_network_latency=True)

    print("\n\n===== Speedup across all experiments =====")
    print("Inter-query median latency speedup over CPU: {:.2f}~{:.2f}x".format(np.min(global_inter_query_median_latency_speedup_over_CPU), np.max(global_inter_query_median_latency_speedup_over_CPU)))
    print("Intra-query median latency speedup over CPU: {:.2f}~{:.2f}x".format(np.min(global_intra_query_median_latency_speedup_over_CPU), np.max(global_intra_query_median_latency_speedup_over_CPU)))
    print("Inter-query P95 latency speedup over CPU: {:.2f}~{:.2f}x".format(np.min(global_inter_query_P95_latency_speedup_over_CPU), np.max(global_inter_query_P95_latency_speedup_over_CPU)))
    print("Intra-query P95 latency speedup over CPU: {:.2f}~{:.2f}x".format(np.min(global_intra_query_P95_latency_speedup_over_CPU), np.max(global_intra_query_P95_latency_speedup_over_CPU)))

    if "HNSW" in graph_types:
        print("Inter-query median latency speedup over GPU: {:.2f}~{:.2f}x".format(np.min(global_inter_query_median_latency_speedup_over_GPU), np.max(global_inter_query_median_latency_speedup_over_GPU)))
        print("Intra-query median latency speedup over GPU: {:.2f}~{:.2f}x".format(np.min(global_intra_query_median_latency_speedup_over_GPU), np.max(global_intra_query_median_latency_speedup_over_GPU)))
        print("Inter-query P95 latency speedup over GPU: {:.2f}~{:.2f}x".format(np.min(global_inter_query_P95_latency_speedup_over_GPU), np.max(global_inter_query_P95_latency_speedup_over_GPU)))
        print("Intra-query P95 latency speedup over GPU: {:.2f}~{:.2f}x".format(np.min(global_intra_query_P95_latency_speedup_over_GPU), np.max(global_intra_query_P95_latency_speedup_over_GPU)))

    # take the max speedup of two array for each element
    Falcon_median_speedup_over_CPU = np.maximum(global_inter_query_median_latency_speedup_over_CPU, global_intra_query_median_latency_speedup_over_CPU)
    Falcon_P95_speedup_over_CPU = np.maximum(global_inter_query_P95_latency_speedup_over_CPU, global_intra_query_P95_latency_speedup_over_CPU)
    print("Falcon (best of inter/intra-query) median latency speedup over CPU: {:.2f}~{:.2f}x".format(np.min(Falcon_median_speedup_over_CPU), np.max(Falcon_median_speedup_over_CPU)))
    print("Falcon (best of inter/intra-query) P95 latency speedup over CPU: {:.2f}~{:.2f}x".format(np.min(Falcon_P95_speedup_over_CPU), np.max(Falcon_P95_speedup_over_CPU)))
    if "HNSW" in graph_types:
        Falcon_median_speedup_over_GPU = np.maximum(global_inter_query_median_latency_speedup_over_GPU, global_intra_query_median_latency_speedup_over_GPU)
        Falcon_P95_speedup_over_GPU = np.maximum(global_inter_query_P95_latency_speedup_over_GPU, global_intra_query_P95_latency_speedup_over_GPU)
        print("Falcon (best of inter/intra-query) median latency speedup over GPU: {:.2f}~{:.2f}x".format(np.min(Falcon_median_speedup_over_GPU), np.max(Falcon_median_speedup_over_GPU)))
        print("Falcon (best of inter/intra-query) P95 latency speedup over GPU: {:.2f}~{:.2f}x".format(np.min(Falcon_P95_speedup_over_GPU), np.max(Falcon_P95_speedup_over_GPU)))
    