#!/usr/bin/env python2

from __future__ import print_function
from collections import defaultdict, Counter, namedtuple
import sys
import itertools
import random

import networkx as nx
import re
import StringIO
import scipy.misc

import walker.data

# Constraints:
#  - Must have > 10 in group
#  - File path rules

#Function = namedtuple('Function', 'name path')

class Function:
    def __init__(self, name=None, path=None):
        self.name = name
        self.path = path

    def __eq__(self, other):
        return self.name == other.name

    def __hash__(self):
        return hash(self.name)


Split = namedtuple('Split', 'prefix suffix')
# TODO?: Replace with just a map from suffix to functions
SuffixGroup = namedtuple('SuffixGroup', 'suffix functions')
PrefixGroup = namedtuple('PrefixGroup', 'prefix functions')
Keyword = namedtuple('Keyword', 'keyword path')
ReviewFile = namedtuple('ReviewFile', 'content path')

GoldenInput = namedtuple('GoldenInput', 'use_underscore use_bootstrap clusters_file reviewed_files')
NewMetricOutput = namedtuple('NewMetricOutput', 'true_positives false_positives true_negatives false_negatives num_must_relations num_mustnot_relations num_missing_vocab missing_functions')

class GoldenOutput:
    def __init__(self, must_equiv_classes, num_true_relations, num_false_relations, must_not_lists):
        self.must_equiv_classes = must_equiv_classes
        self.num_true_relations = num_true_relations
        self.num_false_relations = num_false_relations
        self.must_not_lists = must_not_lists

        self.false_seen = set()

    def num_related_pairs(self):
        total = 0
        for c in self.must_equiv_classes:
            total += scipy.misc.comb(len(c), 2)
        return int(total)

    def related_pairs(self):
        for c in self.must_equiv_classes:
            for u, v in itertools.combinations(c, 2):
                yield (u, v)

    # Close enough for right now, I don't think this is accurate.
    # This is just used for estimation so we aren't going to churn through 2billion pairs
    def estimate_unrelated_pairs(self):
        total = 0
        for must_not_list in self.must_not_lists:
            for must_not_combo in itertools.combinations(must_not_list, 2):
                total += len(must_not_combo[0]) * len(must_not_combo[1])
        total += 1
        return total

    def unrelated_pairs(self):
        # Map each function to its equivalence class.
        # This way we can expand the false relations as so:
        # A must-not B ^ A must C -> C must-not B
        # TODO
        fn_to_equiv = dict()
        for c in self.must_equiv_classes:
            for fn in c:
                fn_to_equiv[fn] = c

        for must_not_list in self.must_not_lists:
            for must_not_combo in itertools.combinations(must_not_list, 2):
                for must_not1, must_not2 in itertools.product(must_not_combo[0], must_not_combo[1]):
                    if must_not1 == must_not2:
                        continue
                    try:
                        not_c1 = fn_to_equiv[must_not1]
                    except:
                        not_c1 = [must_not1]
                    try:
                        not_c2 = fn_to_equiv[must_not2]
                    except:
                        not_c2 = [must_not2]
                    for n_fn1, n_fn2 in itertools.product(not_c1, not_c2):
                        assert n_fn1 != n_fn2
                        if (n_fn1, n_fn2) not in self.false_seen and (n_fn2, n_fn1) not in self.false_seen:
                            self.false_seen.add((n_fn1, n_fn2))
                            yield (n_fn1, n_fn2)

    def implicit_unrelated_pairs(self):
        fn1 = None
        fn2 = None

        # Don't ask for too many :) -> hang
        while True:
            classes = list(random.sample(self.must_equiv_classes, 2))
            fn1 = random.sample(classes[0], 1)[0]
            fn2 = random.sample(classes[1], 1)[0]
            if (fn1, fn2) in self.false_seen or (fn2, fn1) in self.false_seen:
                continue
            self.false_seen.add((fn1, fn2))
            yield (fn1, fn2)


class MergeFunction:
    def __init__(self, name, path):
        self.name = name
        self.path = path

    def __eq__(self, other):
        return self.name == other.name

    def __hash__(self):
        return hash(self.name)

    def __repr__(self):
        return self.name

def parse_reviewed(reviewed_files):
    """
    :param reviewed_files: list of ReviewFile namedtuples
    :return: (must_lists, must_not_lists)
    """
    # Concatenate all of the files and   print("foo")
    must_lists = []
    must_not_lists = []
    for f in reviewed_files:
        # Read lines and remove surround whitespace
        lines = [x.strip() for x in f.content.split('\n')]

        # Remove whitespace between sets for splitting
        lines = [re.sub('}\s*{', '}{', x) for x in lines]

        # For each line
        for l in xrange(len(lines)):
            line = lines[l]
            if not lines[l]:
                continue

            # Must start with one of these characters
            if line[0].upper() == "T":
                must = True
            elif line[0].upper() == "F":
                must = False
            elif line[0].upper() == "#":
                continue
            else:
                raise Exception("Malformed line: {}".format(line))

            sets_line = re.split('}{', line[2:])
            if len(sets_line) == 1:
                # Strip off { .. }
                sets_line[0] = sets_line[0][1:-1]
                # Duplicate omitted set
                sets_line.append(sets_line[0])
            else:
                sets_line[0] = sets_line[0][1:]
                sets_line[-1] = sets_line[-1][:-1]
            sets_line = [x.split() for x in sets_line]

            all_sets = []
            for s in sets_line:
                set_n = [MergeFunction(x, "{}:{}".format(f.path, l+1)) for x in s]
                all_sets.append(set_n)

            if must:
                must_lists.append(tuple(all_sets))
            else:
                must_not_lists.append(tuple(all_sets))

    return must_lists, must_not_lists


def merge(must_lists, must_not_lists):
    """
    Transitive closure of must relations, graph nodes are Function namedtuples

    :param reviewed_files: list of ReviewFile namedtuples
    :param file_paths: list of paths to files (daniel.txt, cindy.txt, aditya.txt)
    :param out: a file-like object
    """
    G = nx.Graph()
    for sublist in must_lists:
        for sublist_combo in itertools.combinations(sublist, 2):
            for u, v in itertools.product(sublist_combo[0], sublist_combo[1]):
                G.add_edge(u, v)
    must_equiv_classes = list(nx.connected_components(G))

    # Check consistency with must_not lists
    # For each must equivalence class
    for must_equiv_class, must_not_list in itertools.product(must_equiv_classes, must_not_lists):
        # Check every pair
        for must_not_combo in itertools.combinations(must_not_list, 2):
            for must_not1, must_not2 in itertools.product(must_not_combo[0], must_not_combo[1]):
                if must_not1 == must_not2:
                    continue
                if must_not1 in must_equiv_class and must_not2 in must_equiv_class:
                    formatted_not = "{} {}".format(must_not1.name, must_not2.name)
                    formatted_equiv_class = " ".join([x.name for x in must_equiv_class])

                    # Compute witness. Inefficient node lookup, but oh well.
                    # Extracting the node keys allows us to get original file path of true relation line
                    for node in G.nodes.keys():
                        if node == must_not1:
                            source = node
                        elif node == must_not2:
                            target = node
                    witness = nx.shortest_path(G, source, target)

                    # This needs to go into the output file
                    out = StringIO.StringIO()
                    out.write("Inconsistency detected!\n")
                    out.write("Must not be related: {}\n".format(formatted_not))
                    out.write("Equivalence class: {}\n".format(formatted_equiv_class))

                    out.write("Witness:\n")
                    witness_it = iter(witness)
                    out.write("(F) {} {}\n".format(must_not1.name, must_not1.path))
                    for node in witness_it:
                        out.write("{} {}\n".format(node.name, node.path))

                    raise Exception(out.getvalue())

    # Consistent.
    return must_equiv_classes


def splits_for_function(function_name):
    """
    Returns all of the prefix/suffix splits to use for a single function name.

    :param function_name: string name of function
    :return: possibly empty list of Split namedtuples (.prefix, .suffix)
    """
    num_underscores = function_name.count('_')
    if num_underscores == 0:
        return []

    splits = []
    for i in range(num_underscores):
        prefix = "_".join(function_name.split('_')[:i+1])
        suffix = "_".join(function_name.split('_')[i+1:])
        if len(prefix) > 1 and len(suffix) > 1:
            splits.append(Split(prefix=prefix, suffix=suffix))

    return splits


def group_by_suffix(functions):
    """
    Groups list of functions by suffix.

    :param functions: List of function namedtuples (.name, .path)
    :return: (List of PrefixGroups, List of SuffixGroups)
    """
    prefix_groups = dict()
    suffix_groups = dict()
    for f in functions:
        for split in splits_for_function(f.name):
            if split.suffix not in suffix_groups:
                suffix_groups[split.suffix] = []
            if split.prefix not in prefix_groups:
                prefix_groups[split.prefix] = []
            suffix_groups[split.suffix].append(f)
            prefix_groups[split.prefix].append(f)

    list_of_suffix_groups = []
    for k in suffix_groups:
        list_of_suffix_groups.append(SuffixGroup(suffix=k, functions=suffix_groups[k]))

    return (prefix_groups, list_of_suffix_groups)


def common_keywords(defined_functions):
    """
    Return a list of Keyword tuples
    """
    paths_to_fns = defaultdict(list)

    # Create a map from file path to the functions defined in that file
    for fn in defined_functions:
        paths_to_fns[fn.path].append(fn.name)

    # Count how often a function name prefix is used in each file
    paths_to_prefix_counts = dict()
    for path in paths_to_fns.keys():
        for fn in paths_to_fns[path]:
            for split in splits_for_function(fn):
                if path not in paths_to_prefix_counts:
                    paths_to_prefix_counts[path] = Counter()
                else:
                    paths_to_prefix_counts[path][split.prefix] += 1

    # Create the list of top 3 prefixes
    common_keywords = dict()
    for path in paths_to_prefix_counts:
        top3_for_file = paths_to_prefix_counts[path].most_common(3)
        if not top3_for_file:
            continue
        for prefix in top3_for_file:
            name, count = prefix[0], prefix[1]
            if count < 10:
                continue
            if prefix[0]  in common_keywords:
                common_keywords[prefix[0]].append(path)
            else:
                common_keywords[prefix[0]] = [path]

    return common_keywords


def filter_groups_by_keyword(suffix_groups, include, exclude, threshold=5):
    """
    :param suffix_groups: List of suffix_groups to filter
    :param include: List of keyword tuples (the path acts as a scope in which the keyword is applicable)
    :param exclude: List of keyword tuples. Exclusion takes precedence over inclusion.
    :param threshold: Minimum number of occurrences of the keyword
    """
    out_suffix_groups = []
    for group in suffix_groups:
        new_group = SuffixGroup(suffix=group.suffix, functions=set())

        for fn in group.functions:
            # "_" is added so that the prefix does not have a trailing underscore
            prefix = fn.name.split("_" + group.suffix)[0]

            if prefix in include and prefix not in exclude:
                if fn.path in include[prefix]:
                    new_group.functions.add(fn)

        if len(new_group.functions) < threshold:
            continue
        out_suffix_groups.append(new_group)

    return out_suffix_groups


def filter_groups_by_path(groups):
    out_groups = []

    for g in groups:
        first_dirs = defaultdict(set)
        for fn in g.functions:
            first_dir = fn.path.split('/')[0]
            first_dirs[first_dir].add(fn)
        for d in first_dirs.keys():
            if len(first_dirs[d]) > 1:
                out_groups.append(SuffixGroup(suffix=g.suffix, functions=first_dirs[d]))

    return out_groups


def bootstrap_metrics(bootstrap_path, groups):
    """
    Reads the list of boostrap functions so that we know the overlap with the golden set.
    :param path: Path to bootstrap file (pathgen -l output)
    :return: The set of all functions in bootstrap file
    """
    with open(bootstrap_path, 'r') as f:
        lines = [x.strip().split() for x in f.readlines()]

    bootstrap_functions = set([item for sublist in lines for item in sublist])
    golden_functions = set()
    for group in groups:
        for fn in group.functions:
            golden_functions.add(fn.name)

    golden_and_bootstrap = golden_functions.intersection(bootstrap_functions)
    print("%s golden, %s golden and bootstrap" % (len(golden_functions), len(golden_and_bootstrap)), file=sys.stderr)


def underscore_wrappers(defined_functions, double=True, single=False):
    """Return list of function pairs of the form __foo, foo or _bar, bar
    The format for each pair is (foo, __foo), where foo and __foo and Function namedtuples.

    This is used by golden.golden.fscore and golden.golden.names

    :param double: include double underscores __foo, foo
    :param single: include single underscores _bar, bar
    """
    single_underscore_names = set()
    double_underscore_names = set()
    all_names = dict()

    for fn in defined_functions:
        all_names[fn.name] = fn
        if fn.name.startswith('__'):
            double_underscore_names.add(fn)
        elif fn.name.startswith('_'):
            single_underscore_names.add(fn)

    ret = []
    if single:
        for fn in single_underscore_names:
            stripped_name = fn.name[1:]
            if stripped_name in all_names:
                ret.append((all_names[stripped_name], fn))
    if double:
        for fn in double_underscore_names:
            stripped_name = fn.name[2:]
            if stripped_name in all_names:
                ret.append((all_names[stripped_name], fn))

    return ret

def golden(golden_input):
    reviewed_files = golden_input.reviewed_files

    rf_tuples = []
    for filepath in reviewed_files:
        f = open(filepath, 'r') 
        text = f.read()
        rf = ReviewFile(content=text, path=filepath)
        f.close()
        rf_tuples.append(rf)

    computed_golden_set = StringIO.StringIO()
    must_lists, must_not_lists = parse_reviewed(rf_tuples)

    must_equiv_classes = walker.golden.merge(must_lists, must_not_lists)

    # Get the number of pairwise true relations in the golden set
    num_true_relations = 0
    for sublist in must_lists:
        for sublist_combo in itertools.combinations(sublist, 2):
            for u, v in itertools.product(sublist_combo[0], sublist_combo[1]):
                num_true_relations += 1

    # Get the number of pairwise false relations in the golden set
    num_false_relations = 0
    for sublist in must_not_lists:
        for sublist_combo in itertools.combinations(sublist, 2):
            for u, v in itertools.product(sublist_combo[0], sublist_combo[1]):
                num_false_relations += 1

    ret = GoldenOutput(
        must_equiv_classes = must_equiv_classes,
        must_not_lists = must_not_lists,
        num_true_relations = num_true_relations,
        num_false_relations = num_false_relations
    )

    return ret


def new_metric(golden_input, clusters):
    """Interpretation
    """

    golden_output = golden(golden_input)

    # Map each function in model to its cluster
    # Clusters use STRING NAMES of functions
    cluster_idx = dict()
    for c in clusters:
        for fn in c:
            cluster_idx[fn] = c

    # In same cluster in model, and in same class in golden set
    true_positives = 0

    # Not in same cluster in model, and false relation in golden set
    true_negatives = 0

    # In same cluster in model, and false relation in golden set
    false_positives = 0

    # Not in same cluster in model, in same class in golden set
    false_negatives = 0

    # True positives
    # For each golden equivalence class
    # Equiv classes use FUNCTION NAMED TUPLES
    num_must_relations = 0
    num_missing_vocab = 0
    missing_functions = set()
    for c in golden_output.must_equiv_classes:
        # For each pair
        for u, v in itertools.combinations(c, 2):
            num_must_relations += 1
            if u.name not in cluster_idx or v.name not in cluster_idx:
                num_missing_vocab += 1
                if u.name not in cluster_idx:
                    missing_functions.add(u)
                elif v.name not in cluster_idx:
                    missing_functions.add(v)
                continue
            if v.name in cluster_idx[u.name]:
                # If they are in the same cluster, then this is a true positive
                true_positives += 1
            else:
                # They are in different clusters, so this is a false negative
                false_negatives += 1

    # For each false relation pair
    num_mustnot_relations = 0
    for sublist in golden_output.must_not_lists:
        for sublist_combo in itertools.combinations(sublist, 2):
            for u, v in itertools.product(sublist_combo[0], sublist_combo[1]):
                num_mustnot_relations += 1
                if u.name not in cluster_idx or v.name not in cluster_idx:
                    num_missing_vocab += 1
                    if u.name not in cluster_idx:
                        missing_functions.add(u)
                    elif v.name not in cluster_idx:
                        missing_functions.add(v)
                    continue
                if v.name in cluster_idx[u.name]:
                    # In the same cluster, false relation
                    false_positives += 1
                else:
                    true_negatives += 1

    return NewMetricOutput(
        true_positives=true_positives,
        false_positives=false_positives,
        true_negatives=true_negatives,
        false_negatives=false_negatives,
        num_must_relations=num_must_relations,
        num_mustnot_relations=num_mustnot_relations,
        num_missing_vocab=num_missing_vocab,
        missing_functions=missing_functions
    )
