import numpy as np
import pandas as pd

from walker.metric import _labeled_and_clustered
from walker.metric import _precision
import walker.golden
from collections import namedtuple

from walker.metric import _recall
from walker.metric import _F
from walker.metric import _Fscore
from walker.metric import _avg_precision
from walker.metric import _avg_recall

Golden = namedtuple('Golden', 'functions f cluster precision recall extra missing')
Split = namedtuple('Split', 'prefix suffix')


# We do not force a 1:1 mapping because our golden set is a subset of functions clustered
# We are evaluating our performance on the golden set, not each cluster.
# Consider the case of C1=ABCDEF, C2=XYZ, T1=ABC, T2=DEF.
# Map T1 to C1, T2 to C2? That's not fair and does not get at what our golden set is trying to do.
def purity(classes, clusters):
    """
    Full precision / recall / purity numbers
    :param classes: A dictionary of all functions and their reference class IDs {function: class},
    :param clusters: A list of sets
    """

    # IMPORTANT: reference_sets is filtered, label_sets is not.
    #filtered_reference, golden_standard, filtered_clusters = _labeled_and_clustered(classes, clusters)
    golden_standard, _, filtered_clusters = _labeled_and_clustered(classes, clusters)

    golden_standard = [x for x in golden_standard if len(x) > 1]

    num_clusters = len(filtered_clusters)
    num_ref = len(golden_standard)

    per_class_precision = np.zeros((num_ref, num_clusters))
    per_class_recall = np.zeros((num_ref, num_clusters))
    per_class_F = np.zeros((num_ref, num_clusters))

    for r in range(0, num_ref):
        for c in range(0, num_clusters):
            per_class_recall[r][c] = _recall(filtered_clusters[c], golden_standard[r])
            per_class_precision[r][c] = _precision(filtered_clusters[c], golden_standard[r])
            per_class_F[r][c] = _F(filtered_clusters[c], golden_standard[r])

    precision_df = pd.DataFrame(per_class_precision)
    recall_df = pd.DataFrame(per_class_recall)
    f_df = pd.DataFrame(per_class_F)

    # DataFrames are indexed COLUMN, ROW

    # Row index -> Matched cluster
    golden_to_cluster = dict()
    for r in range(num_ref):
        max_in_row = 0.0
        for c in range(num_clusters):
            score = f_df.iloc[r, c]
            if score >= max_in_row:
                max_in_row = score
                golden_to_cluster[r] = c

    max_f = f_df.apply(max, axis=1)

    N = len(reduce(lambda x, y: x.union(y), golden_standard))
    f_score = pd.DataFrame([_Fscore(golden_standard, filtered_clusters, N)])
    avg_precision = pd.DataFrame([_avg_precision(golden_standard, filtered_clusters, N)])
    avg_recall = pd.DataFrame([_avg_recall(golden_standard, filtered_clusters, N)])

    sorted_goldens = []
    for i in range(len(max_f)):
        cluster = filtered_clusters[golden_to_cluster[i]]
        category = golden_standard[i]
        missing = set(category).difference(cluster)
        extra = set(cluster).difference(category)
        sorted_goldens.append(Golden(f=max_f[i], functions=golden_standard[i],
                                     cluster=cluster,
                                     precision=_precision(cluster, category),
                                     recall=_recall(cluster, category),
                                     missing=missing,
                                     extra=extra
                                     )
        )
    sorted_goldens.sort(key=lambda g: g.f)
    #sorted_goldens.sort(key=lambda g: (1.0 - g.f) * len(g.functions), reverse=True)

    return (precision_df, recall_df, f_df, golden_standard, filtered_clusters, max_f, f_score, sorted_goldens,
            avg_precision, avg_recall)

def read_functions(defined_functions_file):
    """
    :param defined_functions_file_path: path to output from definedfunctions LLVM pass
    :return: list of namedtuples
    """
    lines = [x.strip() for x in defined_functions_file.readlines()]
    fns_paths = [(x.split()[0], x.split()[1]) for x in lines]

    functions = []
    for path, name in fns_paths:
        functions.append(walker.golden.Function(name=name, path=path))
    return functions

def split_function_with_keyword(function_name, keyword):
   """
   Splits a function name using a specific keyword. The split is performed at the end of the keyword,
   yielding a prefix / suffix split of (*_)keyword_suffix

   :param function_name: string name of function
   :return: None or a Split namedtuple
   """
   num_underscores = function_name.count('_')
   if num_underscores == 0:
       return None

   name_parts = function_name.split("_")
   try:
       idx = name_parts.index(keyword)
   except ValueError:
       return None

   prefix = "_".join(name_parts[:idx+1])
   suffix = "_".join(name_parts[idx+1:])

   return Split(prefix, suffix)
