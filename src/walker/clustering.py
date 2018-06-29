import random

from collections import defaultdict
from gensim.models import KeyedVectors
from sklearn.cluster import DBSCAN
from sklearn.cluster import KMeans

# All clustering methods should return a dictionary of the form
# {cluster_id: [labels]}

def kmeans(word_vectors, k):
    """
    :param word_vectors: gensim.models.KeyedVectors
    :param k: The number of clusters
    :return: ({cluster_id: [cluster elements]}, sklearn.cluster.KMeans)
    """
    word_vectors.init_sims()
    norm_vectors = word_vectors.syn0norm

    kmeans = KMeans(n_clusters=k)
    kmeans.fit(norm_vectors)

    clusters = defaultdict(list)
    for idx in range(0, len(word_vectors.index2word)):
        clusters[kmeans.labels_[idx]].append(word_vectors.index2word[idx])

    return (clusters, kmeans)

def nltk_kmeans(word_vectors, k):
    from nltk.cluster import KMeansClusterer
    import nltk

    #word_vectors.init_sims()
    norm_vectors = word_vectors.syn0

    kmeans = KMeansClusterer(k, nltk.cluster.util.cosine_distance, repeats=25)
    assigned_clusters = kmeans.cluster(norm_vectors, assign_clusters=True)

    clusters = defaultdict(list)
    for idx in range(0, len(word_vectors.index2word)):
        clusters[assigned_clusters[idx]].append(word_vectors.index2word[idx])

    return (clusters, kmeans)

def hierarchical(word_vectors, k):
    from sklearn.cluster import AgglomerativeClustering

    clusterer = AgglomerativeClustering(n_clusters=k, affinity="cosine", linkage="average")
    clusterer.fit(word_vectors.syn0)

    clusters = defaultdict(list)
    for idx in range(0, len(word_vectors.index2word)):
        clusters[clusterer.labels_[idx]].append(word_vectors.index2word[idx])

    return (clusters, clusterer)


def dbscan(word_vectors, eps, min_samples):
    import numpy as np

    print(eps, min_samples)

    word_vectors.init_sims()
    norm_vectors = word_vectors.syn0norm

    #similarities = np.dot(norm_vectors, norm_vectors.T)

    dbscan = DBSCAN(eps=eps, min_samples=min_samples)
    dbscan.fit(norm_vectors)
    print(dbscan.labels_)

    clusters = defaultdict(list)

    for idx in range(0, len(word_vectors.index2word)):
        clusters[dbscan.labels_[idx]].append(word_vectors.index2word[idx])

    return (clusters, dbscan)

def random_k(word_vectors, k):
    """
    :return: ({cluster_id: [cluster elements]})
    """
    clusters = defaultdict(list)
    for word in word_vectors.vocab:
        chosen_cluster = random.randint(0, k-1)
        clusters[chosen_cluster].append(word)

    return clusters
