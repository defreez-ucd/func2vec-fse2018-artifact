from __future__ import print_function
import networkx as nx
from sklearn.manifold import TSNE
import matplotlib.pyplot as plt
from collections import defaultdict
from collections import Counter
import numpy as np

def show_or_save(fn):
    def wrapper(*args, **kwargs):
        ret = fn(*args, **kwargs)
        output_file = kwargs.get('output_file')
        if output_file:
            plt.savefig(output_file)
        else:
            plt.show()

    return wrapper


@show_or_save
def series(x=None, y=None, xlabel=None, ylabel=None, xlim=None, ylim=None, output_file=None):
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)

    if xlim:
        plt.xlim(xlim)
    if ylim:
        plt.ylim(ylim)

    plt.plot(x, y, marker='o')

def dump_graphviz(G, output_file):
    labels = {}
    for u, v, d in G.edges(data=True):
        if d["label"]:
            labels[(u, v)] = d["label"]

    agraph = nx.nx_agraph.to_agraph(G)

    # TODO: We are going to do this a lot. Make a decorator.
    if output_file:
        f = open(output_file, 'w')
        print(agraph, file=f)
        f.close()
    else:
        print(agraph)


def tsne(model, input_file, output_file, perplexity):
    # Read the labels from the input file
    f = open(input_file, 'r')
    lines = f.readlines()
    f.close()

    class_to_labels = defaultdict(list)
    for line in lines:
        label, label_class = line.strip().split()
        if label in model:
            class_to_labels[label_class].append(label)
        else:
            print("WARNING: %s not in model" % label)

    # Dataset
    X = []

    # Where in the tsne table these two functions are stored
    snd_atiixp_free_index = None
    snd_intel8x0free_index = None

    # The two points we label for the paper figure
    snd_atiixp_free_coords = None
    snd_intel8x0free_coords = None

    # Boundary is edge of interval [start, stop) for convenient use with range
    # Exclusive, the stop row is boundary value
    boundaries = []
    row = 0
    row_to_label = []
    for label_class in class_to_labels.keys():
        for label in class_to_labels[label_class]:
            if label == "snd_atiixp_free":
                snd_atiixp_free_index = row
            elif label == "snd_intel8x0_free":
                snd_intel8x0_free_index = row

            X.append(model[label])
            row_to_label.append(label)
            row += 1
        boundaries.append([label_class, row])

    tsne = TSNE(n_components=2, perplexity=perplexity, random_state=0, init='pca')
    X_tsne = tsne.fit_transform(X)

    snd_atiixp_free_coords = X_tsne[snd_atiixp_free_index]
    snd_intel8x0_free_coords = X_tsne[snd_intel8x0_free_index]

    plt.style.use("ggplot")
    plt.rcParams['axes.edgecolor'] = "#777777"
    plt.rcParams['axes.facecolor'] = '#FFFFFF'
    fig, ax = plt.subplots()
    ax.set_xticks([])
    ax.set_yticks([])

    to_legend = ["free", "remove", "probe", "prepare", "open"]

    # Create a separate scatter plot for each class.
    # Creating a single scatter plot and coloring the points might be easier?
    prev_boundary = 0
    legend = []
    legend_names = []
    event_label_lookup = {}
    marker_size = 64
    for boundary in boundaries:
        next_boundary = boundary[1]
        class_results_x = X_tsne[prev_boundary:next_boundary, 0]
        class_results_y = X_tsne[prev_boundary:next_boundary, 1]

        if boundary[0] == "free":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, marker='v', color='red', s=marker_size)
        elif boundary[0]  == "remove":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, marker='^', color='blue', s=marker_size)
        elif boundary[0] == "probe":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, marker='+', color='purple', s=marker_size+25)
        elif boundary[0] == "prepare":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, marker='x', color='orange', s=marker_size)
        elif boundary[0] == "open":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, marker='D', color='green', s=marker_size)
        else:
            other_results_scatter = class_results_scatter = ax.scatter(class_results_x, class_results_y, marker='.', color='lightgray', s=marker_size)

        event_label_lookup[class_results_scatter] = row_to_label[prev_boundary:next_boundary]
        if boundary[0] in to_legend:
            legend.append(class_results_scatter)
            legend_names.append(boundary[0])
        prev_boundary = next_boundary

    legend.append(other_results_scatter)
    legend_names.append('other')

    legend = tuple(legend)
    legend_names = tuple(legend_names)

    leg = plt.legend(legend,
                     legend_names,
                     scatterpoints=1,
                     loc='lower left',
                     ncol=1,
                     fontsize=16,
                     frameon=True,
    )
    leg.get_frame().set_edgecolor('black')
    leg.get_frame().set_facecolor('none')

    ax.annotate('snd_atiixp_free', xy=snd_atiixp_free_coords, xytext=(snd_atiixp_free_coords[0]+10, snd_atiixp_free_coords[1]+10), arrowprops=dict(facecolor='black', shrink=0.05, width=2), fontsize=16)
    ax.annotate('snd_intel8x0_free', xy=snd_intel8x0_free_coords, xytext=(snd_intel8x0_free_coords[0]+10, snd_intel8x0_free_coords[1]+10), arrowprops=dict(facecolor='black', shrink=0.05, width=2), fontsize=16)

    if output_file:
        plt.savefig(output_file)
    else:
        # Doesn't work anymore, unknown reason
        # def onpick(event):
            #artist = event.artist
            #ind = event.ind
            #print('Picked labels: ', [event_label_lookup[artist][x] for x in ind])

        #fig.canvas.mpl_connect('pick_event', onpick)
        plt.tight_layout()
        plt.show()

def simhist(word_vectors, word_vectors2=None):
    vecs = [word_vectors]

    alpha = 1.0
    if word_vectors2:
        vecs.append(word_vectors2)
        plt.hold(True)
        alpha = 0.5

    bins = np.arange(0, 1.1, 0.05)
    fig, ax = plt.subplots()
    ax.set_xticks(bins)

    for wv in vecs:
        wv.init_sims()

        # Efficiently produce similarity matrix
        # For L2-normalized vectors, similarity is equivalent to dot product
        similarities = np.dot(wv.syn0norm, wv.syn0norm.T)

        # Extract off-diagonal similarities (do not compare vector with itself)
        similarities = np.extract(1 - np.eye(similarities.shape[0]), similarities)

        # Plot the histogram
        plt.hist(similarities, edgecolor="black", bins=bins, alpha=alpha)

    plt.show()

    # Cumulative similarity plot
    # fig, ax = plt.subplots()
    # bins = np.arange(0, 1.1, 0.05)
    # plt.hist(similarities, edgecolor="black", bins=bins, cumulative=-1)
    # ax.set_xticks(bins)
    # plt.show()



@show_or_save
def plot_walks(walks, label_output_file, length_output_file):
    counter = Counter()
    lengths = []
    for walk in walks:
        counter.update(walk)
        lengths += [len(walk)]
    plt.hist(counter.values(), log=True, bins=np.logspace(0, 3.0, 100))
    plt.xlabel("Node occurrence")
    plt.ylabel("Frequency")
    plt.gca().set_xscale("log")

    if (label_output_file):
        plt.savefig(label_output_file)
    else:
        plt.show()
    plt.clf()

    plt.hist(lengths, log=True, bins=np.logspace(0, 3.0, 100))
    plt.xlabel("Walk length")
    plt.ylabel("Frequency")
    plt.gca().set_xscale("log")


def plot_call_depth_histogram(pds, output_fig_calls, output_fig_returns):
    fig, ax = plt.subplots()
    plt.title("Max call depth per sentence")
    plt.hist(pds.max_call_depths, edgecolor="black")
    plt.savefig(output_fig_calls)

    fig, ax = plt.subplots()
    plt.title("Max return depth per sentence")
    plt.hist(pds.max_return_depths, edgecolor="black")
    plt.savefig(output_fig_returns)


@show_or_save
def heatmap(heat_values, output_file=None):
    ax = sns.heatmap(heat_values, annot=True)


def dump_characteristics(G, output_prefix):
    def plot_degree_distribution(G, output_prefix):
        """
        Plot log-log graphs about in and out degree distributions.
        """
        plt.hist(G.out_degree().values(), log=True,
                 bins=np.logspace(0, 3.0, 100))
        plt.xlabel("out-degree")
        plt.ylabel("Frequency")
        plt.gca().set_xscale("log")
        outdeg_file = output_prefix + "-out-degree.png"
        print("Saving to", outdeg_file)
        plt.savefig(outdeg_file)
        plt.clf()
        """for k, v in G.out_degree().iteritems():
                if v >= 100:
                        print k, "out ", v
        """
        plt.hist(G.in_degree().values(), log=True,
                 bins=np.logspace(0, 3.0, 100))
        plt.xlabel("in-degree")
        plt.ylabel("Frequency")
        plt.gca().set_xscale("log")
        indeg_file = output_prefix+"-in-degree.png"
        print("Saving to", indeg_file)
        plt.savefig(indeg_file)
        plt.clf()
        """for k, v in G.out_degree().iteritems():
                if v >= 100:
                        print k, "in ", v
        """
    def plot_callsite_distribution(G, output_prefix):
        counter = Counter()
        for u, v, d in G.edges(data=True):
            if d["label"]:
                counter.update([d["label"]])
        print(counter)
        plt.hist(counter.values(), log=True, bins=np.logspace(0, 3.1, 100))
        plt.xlabel("Call sites")
        plt.ylabel("Frequency")
        plt.gca().set_xscale("log")
        file = output_prefix + "-call-sites.png"
        print("Saving to", file)
        plt.savefig(file)
        plt.clf()

    plot_degree_distribution(G, output_prefix)
    plot_callsite_distribution(G, output_prefix)
    # Print raw graphviz for visualization
    # Create a cleaner data map for plotting
