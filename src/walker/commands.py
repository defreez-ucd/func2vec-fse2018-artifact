from collections import defaultdict, Counter
from copy import deepcopy
import csv
import itertools
import functools
import os
import sys
import csv

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

import numpy as np
import walker.pushdown
from walker.pushdown import PushDown
from gensim.models.word2vec import Word2Vec, score_sentence_sg
from gensim.models.keyedvectors import KeyedVectors

import clustering
import data
import metric
import topicmodeling
import visualization
import aws
import config
import golden

# See main for valid parameter configurations.
def walk(bitcode_file=None, output_file=None, length=None, walks=None, getgraph_binary="/program2vec/build/getgraph", error_codes=None, interprocedural=True, flat=False, remove=None, enterexit=False, labels=None, bias_constant=1.0, output_distances=None, remove_cross_folder=False):
    if not flat:
        assert walks and length

    if not remove:
        remove = []

    if labels:
        if labels.errorcodes:
            error_codes = "codes.txt"
        if not labels.types:
            remove.append("F2V_GEP")
        if not labels.opcodes:
            remove.append("F2V_INST")
        if not labels.ifs:
            remove.append("F2V_CONDBR")

    edgelist = walker.pushdown.generate_edgelist(bitcode_file, getgraph_binary, error_codes)
    G = walker.pushdown.create_graph_from_edgelist(edgelist, remove_labels=remove, interprocedural=interprocedural)
    if not flat:
        PDS = PushDown(G, enterexit, interprocedural, bias_constant=bias_constant)
    else:
        PDS = PushDown(G, enterexit, False, bias_constant=1.0)
    if output_file:
        f = open(output_file, 'w')
        if not flat:
            PDS.random_walk_all_labels(length, walks, f)
        else:
            walker.pushdown.flat_walk(G, output_file=f)
        f.close()
    else:
        if not flat:
            PDS.random_walk_all_labels(length, walks, sys.stdout)
        else:
            walker.pushdown.flat_walk(G, output_file=sys.stdout)

    if output_distances:
        with open(output_distances, 'w') as f:
            writer = csv.writer(f)
            writer.writerow(PDS.max_stack_distances)

    return PDS



def train(output_file=None, length=None, walks=None, window=None, mincount=None, paths_file=None,
          getgraph_binary=None, error_codes=None, cache=False, size=None, skipgram=1, hs=0, remove=[]):
    assert paths_file
    assert window and mincount and size
    assert skipgram != hs

    walks_list = _read_walks_from_file(paths_file, remove)
    model = Word2Vec(walks_list, window=window, min_count=mincount, size=size, sg=skipgram, hs=hs)

    if output_file:
        model.wv.save_word2vec_format(output_file, binary=False)

    return model.wv

def aws_endtoend():
    configs = config.Factory.get_configs("arxiv_vmlinux")
    runner = aws.JobRunner()
    for c in configs:
        aws_config = config.AwsConfig(c)
        runner.submit(aws_config)

def aws_terminate():
    runner = aws.JobRunner()
    runner.terminate_all()


def pairsim(model_file=None, label1=None, label2=None):
    model = Word2Vec.load(model_file)
    word_vectors = model.wv

    print(word_vectors.similarity(label1, label2))
    print(word_vectors.most_similar(label1))
    print(word_vectors.most_similar(label2))


def tsne(model_file=None, functionclasses_file=None, output_file=None,
         perplexity=None):
    word_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    visualization.tsne(word_vectors, functionclasses_file, output_file, perplexity)


def simhist(model_file=None, model2_file=None):
    word_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    if model2_file:
        word_vectors2 = KeyedVectors.load_word2vec_format(model2_file, binary=False)
        visualization.simhist(word_vectors, word_vectors2)
    else:
        visualization.simhist(word_vectors)


def graphviz(bitcode_file=None, output_file=None, getgraph_binary=None, error_codes=None):
    edgelist = walker.pushdown.generate_edgelist(bitcode_file, getgraph_binary, error_codes)
    G = walker.pushdown.create_graph_from_edgelist(edgelist)
    visualization.dump_graphviz(G, output_file)
    # Print the id to label mapping. Not sure whether to always print this.
    # stderr.write("Id to label mapping\n")
    # for k, v in G.graph["id_to_label"].iteritems():
    #    stderr.write("%s: %s\n" %(k, v.label))


def _read_walks_from_file(walks_file=None, remove=None):
    if not remove:
        remove = set()

    walks = []
    f = open(walks_file, 'r')
    for line in f:
        line = line.strip()
        walk = []
        for label in line.split():
            add = True
            for r in remove:
                if label.startswith(r):
                    add = False
                    break
            if add:
                walk.append(label)
        walks.append(walk)
    f.close()

    return walks


def plotwalks(walks_file=None, label_output_file=None, length_output_file=None):
    walks = _read_walks_from_file(walks_file)
    visualization.plot_walks(walks, label_output_file, length_output_file)


def kmeans(model_file=None, nclusters=None, output_file=None):
    word_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    clusters = clustering.kmeans(word_vectors, nclusters)[0]

    if output_file:
        out = open(output_file, 'w')
    else:
        out = sys.stdout

    for k in clusters:
        out.write("%s\n" % " ".join([fn for fn in clusters[k]]))

    if output_file:
        out.close()

def cluster_random(model_file=None, k=None, output_file=None):
    word_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    clusters = clustering.random_k(word_vectors, k)

    with open(output_file, 'w') as out:
        for c in clusters:
            out.write("%s\n" % " ".join([fn for fn in clusters[c]]))


def dbscan(model_file=None, eps=None, output_file=None, min_samples=None):
    word_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    clusters = clustering.dbscan(word_vectors, eps, min_samples)[0]

    if output_file:
        out = open(output_file, 'w')
    else:
        out = sys.stdout

    for k in clusters:
        out.write("%s\n" % " ".join([fn for fn in clusters[k]]))

    if output_file:
        out.close()

def lda(walks_file=None, numtopics=None, output_file=None):
    walks_list = _read_walks_from_file(walks_file)
    topics = topicmodeling.lda(walks_list, numtopics)

    print(topics)


def __read_class_labels(classes_file):
    """
    Returns a function -> class-label dictionary. Only supports single class labels.
    :param functionclasses_file: A file with the format function-name class-label
    :return: Dictionary of the form {fn: class}
    """
    classes = {}
    with open(classes_file, 'r') as f:
        for line in f:
            function, class_label = line.strip().split()
            if function in classes:
                continue
#                raise Exception("Duplicate function %s" % function)
            classes[function] = class_label
    return classes


def logit(model_file=None, classes_file=None, average=None, zoomout=None, output_file=None):
    keyed_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    classes = __read_class_labels(classes_file)

    x_values = np.arange(0.1, 1, 0.1)
    y_values = []
    for x in x_values:
        score = metric.f1_score(classes, average, x, keyed_vectors)
        y_values.append(score)

    if zoomout:
        visualization.series(x=x_values, y=y_values, xlabel='Training fraction', ylabel='%s-F1 score' % average,
                             xlim=(0, 1), ylim=(0, 1), output_file=output_file)
    else:
        visualization.series(x=x_values, y=y_values, xlabel='Training fraction', ylabel='%s-F1 score' % average,
                             output_file=output_file)

def golden_names(functions_file=None, output_file=None, bootstrap_file=None):
    with open(functions_file, 'r') as f:
        defined_functions = data.read_functions(f)

    prefix_groups, suffix_groups = golden.group_by_suffix(defined_functions)
    # Get a list of the most common function name prefixes for each file
    # keywords is a map from keyword to the list of files in which that is a top 3 keyword
    keywords = golden.common_keywords(defined_functions)

    blacklist = (
        # Otherwise ethtool_get_coalesce and ethtool_set_coalesce are put together
        # This was just meant to be an example, but it is useful and we never took it any farther...
#        "ethtool_get",
#        "ethtool_set"
    )

    # Function must start with the prefix and be a keyword that is paired with the correct file
    # Threshold is the minimum number of function names that use the keyword
    keyword_filtered_suffix_groups = golden.filter_groups_by_keyword(suffix_groups, keywords, blacklist, threshold=10)

    # Filter out functions that have differing first directory paths
    path_filtered_suffix_groups = golden.filter_groups_by_path(keyword_filtered_suffix_groups)

    # Print some numbers
    #print("{} suffix groups".format(len(path_filtered_suffix_groups)))
    #bootstrap_metrics("/efs/data/arxiv/inputs/bootstrap.txt", path_filtered_suffix_groups)

    golden.output_golden_set(path_filtered_suffix_groups, one_per_line=True, out_file=output_file)

def data_purity(model_file=None, output_directory=None, clusters_file=None, reference_file=None):
    keyed_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    classes = __read_class_labels(reference_file)
    with open(clusters_file, 'r') as f:
        clusters = [set(x.strip().split()) for x in f.readlines()]

    # Pandas DataFrames
    result = data.purity(classes, keyed_vectors, clusters)
    precision_df = result[0]
    recall_df = result[1]
    f_df = result[2]
    filtered_reference=result[3]
    filtered_clusters=result[4]
    max_f=result[5]
    f_score=result[6]

    filtered_reference = [list(x) for x in filtered_reference]
    filtered_clusters = [list(x) for x in filtered_clusters]

    precision_csv_path = os.path.join(output_directory, "precision.csv")
    recall_csv_path = os.path.join(output_directory, "recall.csv")
    f_csv_path = os.path.join(output_directory, "f.csv")
    max_f_path = os.path.join(output_directory, "max_f.csv")
    f_score_path = os.path.join(output_directory, "f_score.csv")
    metrics_path = os.path.join(output_directory, "metrics.txt")

    precision_df.to_csv(precision_csv_path)
    recall_df.to_csv(recall_csv_path)
    f_df.to_csv(f_csv_path)
    max_f.to_csv(max_f_path)
    f_score.to_csv(f_score_path)

    # Output list of reference clusters sorted by how poorly we do on them

    clusters_csv_path = os.path.join(output_directory, "filtered_clusters.csv")
    reference_csv_path = os.path.join(output_directory, "filtered_reference.csv")
    with open(clusters_csv_path, "w") as f:
        writer = csv.writer(f)
        writer.writerows(filtered_clusters)
    with open(reference_csv_path, "w") as f:
        for ref in filtered_reference:
            f.write("%s\n" % " ".join(ref))

def data_lpds(bitcode_file=None, output_file=None):
    assert bitcode_file

    if output_file:
        f = open(output_file, 'w')
    else:
        f = sys.stdout

    # Works for docker container - might be worth parameterizing
    getgraph_binary = "/program2vec/build/getgraph"

    # Hardcodes not using error codes (double check against paper)
    edgelist = walker.pushdown.generate_edgelist(bitcode_file, getgraph_binary, False)
    G = walker.pushdown.create_graph_from_edgelist(edgelist, remove_labels=None, interprocedural=True)

    labels = set()
    for u, v, d in G.edges(data=True):
        if d["label"]:
            for l in d["label"]:
                labels.add(l)

    f.write("Nodes: %s\n" % len(G.nodes()))
    f.write("Rules: %s\n" % len(G.edges()))
    f.write("Labels: %s\n" % len(labels))


def metric_roc(model_file=None, reference_file=None, png_file=None):
    GOLDEN_INPUT = walker.golden.GoldenInput(
            use_underscore=False,
            use_bootstrap=False,
            clusters_file='',
            reviewed_files = [reference_file]
            )

    golden_params = walker.golden.golden(GOLDEN_INPUT)
    model = KeyedVectors.load_word2vec_format(model_file, binary=False)

    num_true = None
    num_false = None

    scores, sims, true_done, false_done, num_functions = walker.metric.multi_roc(
            [model],
            golden_params,
            ["model"],
            num_true,
            num_false
            )

    figfile = open(png_file, 'w')
    plt.savefig(figfile, format='png')
    figfile.close()

    for s in scores:
        print(s)

def metric_f1(model_file=None, functionclasses_file=None, average=None):
    assert average

    keyed_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    classes = __read_class_labels(functionclasses_file)
    score = metric.f1_score(classes, average, 0.3, keyed_vectors)
    print(score)


def metric_f1cluster(reference_file=None, clusters_file=None, output_dir=None):
    """
    Use this for computing the F1 scores shown in the paper.
    :param output_dir: Directory to put the following files:
                       filtered_clusters.csv
                       reference_sets.csv
                       precision.csv
                       recall.csv
                       purity.csv (todo)
                       inverse_purity.csv (todo)
    """
    GOLDEN_INPUT = walker.golden.GoldenInput(
            use_underscore=False,
            use_bootstrap=False,
            clusters_file=clusters_file,
            reviewed_files = [reference_file]
            )

    golden_output = walker.golden.golden(GOLDEN_INPUT)
    must_equiv_classes = golden_output.must_equiv_classes

    function_classes = dict()
    golden_set_size = 0
    for i in xrange(len(must_equiv_classes)):
        must_equiv_class = must_equiv_classes[i]
        for f in must_equiv_class:
            golden_set_size += 1
            function_classes[f.name] = i

    with open(clusters_file, 'r') as f:
        clusters = [set(x.strip().split()) for x in f.readlines()]

    results = walker.data.purity(function_classes, clusters)

    f_score = results[6][0][0]

    print(f_score)


def metric_analogies(model_file=None, questions_file=None, clusters_file=None):
    word_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    score = metric.analogies(questions_file, clusters_file, word_vectors)
    print(score)


def metric_silhouette(model_file=None, k=None):
    word_vectors = KeyedVectors.load_word2vec_format(model_file, binary=False)
    print(metric.silhouette(k, word_vectors))


def metric_sg(model_file=None, word1=None, word2=None):
    print(model_file)
    model = Word2Vec.load(model_file)

    with open('sound_context_items2.txt') as f:
        context_items = [x.strip() for x in f.readlines()]
    with open('sound_response_items2.txt') as f:
        response_items = [x.strip() for x in f.readlines()]

    vocab = [x for x in model.wv.vocab]

    for pair in itertools.product(context_items, response_items):
        if pair[0] not in vocab or pair[1] not in vocab:
            continue
        print("%s %s %s" % (pair[0], pair[1], metric.score_sg_pair(model, pair[0], pair[1])))

def metric_avgprecision(input=None):
    """
    Format of file is merged, unmerged
    1 1
    1 0
    ... 
    """
    import numpy as np
    with open(input, 'r') as f:
        lines = [x.strip().split() for x in f.readlines()]

    A = np.array(lines)
    A.astype(np.int8)

    merged = A[:,0]
    unmerged = A[:,1]

    assert len(merged) == 150
    assert len(unmerged) == 150

    merged_ap = metric.avgprecision(merged)
    unmerged_ap = metric.avgprecision(unmerged)

    print("Merged: %s, Unmerged: %s" % (merged_ap, unmerged_ap))

def graphcharac(bitcode_file=None, output_prefix=None, getgraph_binary=None, error_codes=None):
    edgelist = walker.pushdown.generate_edgelist(bitcode_file, getgraph_binary, error_codes)
    G = walker.pushdown.create_graph_from_edgelist(edgelist)
    visualization.dump_characteristics(G, output_prefix)
