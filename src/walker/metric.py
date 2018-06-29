import itertools
from operator import itemgetter
import sys
import random

import gensim.matutils
from gensim.models.word2vec import score_sg_pair
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sklearn
from sklearn.metrics import silhouette_score
from sklearn.metrics import roc_auc_score

import clustering
from clustering import kmeans


def multi_roc(models, golden_output, model_names, num_true=None, num_false=None):
    """For overlaying multiple ROC curves on a single plot.
    There has to be a better way to do this.

    :param models: A list of gensim KeyedVectors instances
    :param golden_output: walker.golden.GoldenOutput instance
    :param num_true: The number of true relations to pick
    :param num_false: The number of false relations to pick
    """
    # Convert list of lists to dict fn: label
    #all_vocab = set()
    #for m in models:
    #    m_vocab = set(m.vocab)
    #    if len(all_vocab) == 0:
    #        all_vocab = m_vocab
    #    else:
    #        all_vocab = all_vocab.intersection(m_vocab)

    matplotlib.rcParams.update({'font.size': 14})

    lw = 2
    plt.xlabel('False Positive Rate')
    plt.ylabel('True Positive Rate')
    plt.legend(loc="lower right")
    plt.xlim([0.0, 1.0])
    plt.ylim([0.0, 1.05])

    all_functions = set()

    # Take all 2-combos and compute the similarity of each pair
    model_similarities = []
    scores = []
    linestyles = ["solid", "dotted"]
    for m in range(0, len(models)):
        model = models[m]
        similarities = []

        true_seen = set()
        false_seen = set()
        true_done = 0
        false_done = 0
        true_it = golden_output.related_pairs()
        explicit_false_it = golden_output.unrelated_pairs()
        implicit_false_it = golden_output.implicit_unrelated_pairs()

        while num_true == None or true_done < num_true:
            try:
                fn1, fn2 = next(true_it)
            except StopIteration:
                break
            all_functions.add(fn1)
            all_functions.add(fn2)
            #if fn1.name not in all_vocab or fn2.name not in all_vocab:
            #    continue
            try:
                sim = model.similarity(fn1.name, fn2.name)
            except:
                sim = random.uniform(0, 1)
            similarities.append((fn1.name, fn2.name, sim, True))
            assert (fn1.name, fn2.name) not in true_seen
            true_seen.add((fn1.name, fn2.name))
            true_done += 1

        # 10x as many false
        if num_false == None:
            num_false = true_done * 10
        if true_done and true_done < num_false:
            num_false = true_done * 10

        # TODO: When we run out of explicit false pairs,
        # Then expand with implicit false pairs
        while num_false == None or false_done < num_false:
            try:
                fn1, fn2 = next(explicit_false_it)
            except StopIteration:
                break
            #if fn1.name not in all_vocab or fn2.name not in all_vocab:
            #    continue
            try:
                sim = model.similarity(fn1.name, fn2.name)
            except:
                sim = random.uniform(0, 1)
            similarities.append((fn1.name, fn2.name, sim, False))
            assert (fn1.name, fn2.name) not in false_seen
            false_seen.add((fn1.name, fn2.name))
            false_done += 1

        # If we didn't have enough explicit false pairs,
        # then continue with implicit false pairs.
        while false_done < num_false:
            fn1, fn2 = next(implicit_false_it)
            #if fn1.name not in all_vocab or fn2.name not in all_vocab:
            #    continue
            try:
                sim = model.similarity(fn1.name, fn2.name)
            except:
                sim = random.uniform(0, 1)
            similarities.append((fn1.name, fn2.name, sim, False))
            assert (fn1.name, fn2.name) not in false_seen
            false_seen.add((fn1.name, fn2.name))
            false_done += 1

        # Sort the list by descending similarity
        similarities = sorted(similarities, key=lambda x: x[2], reverse=True)
        model_similarities.append(similarities)

        # counts at rank 1, rank 2, rank 3, etc...
        true_positive_rates = []
        false_positive_rates = []
        cur_tp = 0
        cur_fp = 0
        for i in xrange(len(similarities)):
            if similarities[i][3]:
                cur_tp += 1
            else:
                cur_fp += 1
            true_positive_rates.append(float(cur_tp) / true_done)
            false_positive_rates.append(float(cur_fp) / false_done)

        plt.plot(false_positive_rates, true_positive_rates, lw=lw, label=model_names[m], linestyle=linestyles[m])

        # Return score
        y_true_ones = np.ones(true_done, dtype=np.int)
        y_true_zeros = np.zeros(false_done, dtype=np.int)
        y_true = np.concatenate((y_true_ones, y_true_zeros))
        y_scores = np.array([x[3] for x in similarities], dtype=np.int)

        scores.append(roc_auc_score(y_true, y_scores))

    plt.plot([0, 1], [0, 1], color='navy', lw=lw, linestyle="dashed")
    plt.legend(loc='lower right')

    return (scores, similarities, true_done, false_done, len(all_functions))


# Currently hard-coded to logistic regression
def f1_score(classes, average, train_size, keyedvectors, xlim=None, ylim=None):
    """
    :param classes: Path to fie containing function class labels
    :param average: Averaging method, choices ['micro', 'macro']
    :param train_size: Fraction of function class labels to use for training
    :param keyedvectors: The word vectors
    :param xlim: X axis limit
    :param ylim: Y axis limit
    :return: The F1 score
    """
    X = []
    y = []
    for v in range(0, len(keyedvectors)):
        fn = keyedvectors.index2word[v]

        if fn in classes:
            X.append(keyedvectors)
            y.append(classes[fn])

    X_train, X_test, y_train, y_test = sklearn.model_selection.train_test_split(X, y, train_size=train_size)
    clf = sklearn.linear_model.LogisticRegression()
    # import sklearn.dummy
    #    clf = sklearn.dummy.DummyClassifier(strategy="uniform")
    clf.fit(X_train, y_train)
    y_pred = clf.predict(X_test)

    return sklearn.metrics.f1_score(y_test, y_pred, average=average)

### Helper functions for computing f measures
def _precision(a, b):
    return float(len(a.intersection(b))) / float(len(a))

def _recall(a, b):
    return _precision(b, a)

# Not used, but kept here for reference
# To get inverse purity, call purity(labels, clusters, N)
def _purity(clusters, labels, N):
    res = 0.0
    for c in clusters:
        precision_all = [_precision(c, l) for l in labels]
        _, precision_max_value = max(enumerate(precision_all), key=itemgetter(1))
        res += (float(len(c)) / N) * precision_max
    return res

def _F(label, cluster):
    r = _recall(label, cluster)
    p = _precision(label, cluster)
    if (r + p) == 0.0:
        return 0.0
    f = (2.0 * r * p) / (r + p)
    return f

def _Fscore(labels, clusters, N):
    res = 0.0
    for label in labels:
        # Maximum individual f score for any cluster that applies to this label
        f_all = [_F(label, c) for c in clusters]
        f_max_idx, f_max_value = max(enumerate(f_all), key=itemgetter(1))
        res += (len(label) / float(N)) * f_max_value
    return res

def _avg_precision(labels, clusters, N):
    res = 0.0

    total_in_clusters = 0

    for label in labels:
        # Maximum individual f score for any cluster that applies to this label
        prec_all = [_precision(c, label) for c in clusters]
        prec_max_idx, prec_max_value = max(enumerate(prec_all), key=itemgetter(1))
        total_in_clusters += len(clusters[prec_max_idx])

    for label in labels:
        # Maximum individual f score for any cluster that applies to this label
        prec_all = [_precision(c, label) for c in clusters]
        prec_max_idx, prec_max_value = max(enumerate(prec_all), key=itemgetter(1))
        res += (len(clusters[prec_max_idx]) / float(total_in_clusters)) * prec_max_value
    return res

def _avg_recall(labels, clusters, N):
    res = 0.0
    for label in labels:
        # Maximum individual f score for any cluster that applies to this label
        prec_all = [_recall(c, label) for c in clusters]
        prec_max_idx, prec_max_value = max(enumerate(prec_all), key=itemgetter(1))
        res += (len(label) / float(N)) * prec_max_value
    return res

def _labeled_and_clustered(classes, clusters):
    """
    Filters reference class (sets) and clusters by intersection

    :param classes: A dictionary of all functions and their reference classes {function: class}
    :param clusters: A list of sets
    :return: (A list of filtered reference sets, A list of filtered clusters)
    """
    label_sets_dict = dict()
    for fn, c in classes.iteritems():
        if c not in label_sets_dict:
            label_sets_dict[c] = {fn}
        else:
            label_sets_dict[c].add(fn)

    label_sets = []      # The actual reference class sets. The index matches labels
    for k, v in label_sets_dict.iteritems():
        label_sets.append(v)

    # Set of all items that appear in any cluster
    all_clustered_items = reduce(lambda x, y: x.union(y), [set(c) for c in clusters])
    # Set of all items that appear in any reference class
    all_labeled_items = reduce(lambda x, y: x.union(y), label_sets)

    # Items that are both in a cluster and in a reference class
    labeled_and_clustered = all_clustered_items.intersection(all_labeled_items)

    # We cluster all functions together, but intersect with labeled data for evaluation.
    filtered_clusters = [c.intersection(labeled_and_clustered) for c in clusters]
    filtered_clusters = [c for c in filtered_clusters if c]
    filtered_reference = [c.intersection(labeled_and_clustered) for c in label_sets]
    filtered_reference = [c for c in filtered_reference if c]

    return (filtered_reference, label_sets, filtered_clusters)


def f1_cluster(classes, keyed_vectors, clusters):
    """
    :param classes: A dictionary {function: reference class}
    :param keyedevectors: Gensim word vectors
    :param: clusters: A list of sets
    """
    reference_sets, _, filtered_clusters = _labeled_and_clustered(classes, clusters)

    # Total number of items that are labeled and clustered
    N = len(reduce(lambda x, y: x.union(y), reference_sets))
    f1 = _Fscore(reference_sets, filtered_clusters, N)

    return f1


def analogies(questions_word_file, clusters_file, word_vectors):
    """
    :param questions_words_file:
    :param word_vectors:
    :return: (correct, incorrect)
    """

    # Normal gensim analogies
    # ret = word_vectors.accuracy(questions_words_file)
    # correct = len(ret[0]['correct'])
    # incorrect = len(ret[0]['incorrect'])
    # return (correct, incorrect, float(correct) / (correct + incorrect))

    with open(questions_word_file, 'r') as f:
        questions = [x.strip().split() for x in f.readlines()]

    with open(clusters_file, 'r') as f:
        clusters = [x.strip().split() for x in f.readlines()]

    cluster_dict = {}
    for c in clusters:
        for label in c:
            cluster_dict[label] = c

    total = 0
    correct = 0
    for a, b, c, expected in questions:
        if not a in word_vectors.vocab or not b in word_vectors.vocab:
            continue
        if not c in word_vectors.vocab or not expected in word_vectors.vocab:
            continue

        ignore = set([a, b, c])
        sims = word_vectors.most_similar(positive=[b, c], negative=[a], topn=False)

        for index in gensim.matutils.argsort(sims, reverse=True):
            predicted = word_vectors.index2word[index]
            if predicted not in ignore:
                break

        predicted_cluster = cluster_dict[predicted]
        expected_cluster = cluster_dict[expected]

        if predicted_cluster == expected_cluster:
            correct += 1
        total += 1
    incorrect = total - correct
    return correct, incorrect, float(correct) / (total)


def avgprecision(results):
    """
    :param results: List of True/False values
    """
    total_points = 0.0
    correct = 0.0
    for i, p in enumerate(results):
        position = i + 1
        if p:
            correct += 1
            points = correct / position
            total_points += points

    return total_points / len(results)

def silhouette(k, word_vectors):
    kmeans = kmeans(word_vectors, k)[1]
    return silhouette_score(word_vectors.syn0norm, kmeans.labels_)


def score_sg_pair(model, word1, word2):
    """
    Only meaningful when trained with skip-gram model?
    """
    vocab1 = model.wv.vocab[word1]
    vocab2 = model.wv.vocab[word2]

    return score_sg_pair(model, vocab1, vocab2)
