from __future__ import print_function
import sys
import os
import itertools
import logging
import re
import tempfile

from collections import namedtuple

from ehnfer.handler_db import HandlerDb, Item, ItemType, TacticType, Spec
from specs_spreadsheet import SpecsSpreadsheet
from eclat import Eclat
from association_rule import AssociationRule

from gensim.models.keyedvectors import KeyedVectors

import networkx as nx

from IPython import embed

Spec = namedtuple('Spec', ['rule', 'eclat_support', 'support',
                           'merged_support', 'global_confidence', 'local_confidence'])

def mangle_item(item):
    return item.split('.')[0]
    return item
    if "PRE|kzalloc" in item:
        return item.split('.')[0]
    if "." in item:
        return ""
    return item

def include_sentence(sentence):
    return True
    pci_request_regions = "PRE|pci_request_regions" in sentence
    pci_enable_device = "PRE|pci_enable_device" in sentence
    kzalloc = "PRE|kzalloc" in sentence
    free = re.search('snd_.+_free', " ".join(sentence))

    ret = pci_request_regions and pci_enable_device and kzalloc and free
    return ret

def mine(db_file=None,
         support_threshold=3, similarity_threshold=0.9,
         model_file=None, num_epochs=10, filter=None, output=None):

    assert(output)

    filter = '.*ext4.*'

    # Open the handler database and read handler information
    hdb = HandlerDb(db_file)

#    embed()

    logging.basicConfig(format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p', level=logging.INFO)

    sentences_temp_path = "%s/%s" % (tempfile.gettempdir(), next(tempfile._get_candidate_names()))
    print("SENTENCES AT:", sentences_temp_path, file=sys.stderr)

    # Write out sentences to tmp file
    with open(sentences_temp_path, 'w') as f:
        # For each handler
        for h in hdb.handlers.keys():
            handler = hdb.handlers[h]

            # Filter out sentences that have either an empty context or response
            if len(handler.context) == 0 or len(handler.response) == 0:
                continue

            if not re.match(filter, handler.predicate_loc):
                continue

            sentence = [mangle_item("PRE|" + x.name) for x in handler.context]
            sentence += ([mangle_item("POST|" + x.name) for x in handler.response])

            if include_sentence(sentence):
                f.write(" ".join(sentence))
                f.write("\n")

    # Run eclat frequent itemset mining
    eclat = Eclat()
    eclat.mine(sentences_temp_path, support_threshold)
    eclat_rules = eclat.get_rules()

    model = KeyedVectors.load_word2vec_format(model_file, binary=False)

    print("Merging...", file=sys.stderr)
    merge_classes = merge(eclat_rules, model, similarity_threshold)

    print("Scoring...")
    specs = []

    i = 0
    for c in merge_classes:
        i += 1
        print((float(i) / len(merge_classes)) * 100)
        merged_support = len(hdb.supporting_handlers(list(c)))
        print(c)
        for h in hdb.supporting_handlers(list(c)):
            print(h)
        print()
        for r in c:
            support = len(hdb.supporting_handlers([r]))
            eclat_support = eclat.get_support_for_rule(r)
            global_confidence = hdb.global_confidence(r)
            local_confidence = hdb.local_confidence(r)
            if include_rule(r):
                s = Spec(rule=r, eclat_support=eclat_support, support=support, merged_support=merged_support,
                         global_confidence=global_confidence, local_confidence=local_confidence)
                specs.append(s)

    for r in eclat_rules:
        if include_rule(r):
            eclat_support = eclat.get_support_for_rule(r)
            global_confidence = hdb.global_confidence(r)
            local_confidence = global_confidence
            s = Spec(rule=r, eclat_support=eclat_support, support=None, merged_support=eclat_support,
                     global_confidence=global_confidence, local_confidence=local_confidence)
            specs.append(s)

    # unfiltered
    final_specs = specs

    # filtered
    #final_specs = filter_specs(specs)
    #final_specs = subsume_specs(final_specs)

    final_specs.sort(key=lambda x: x.merged_support, reverse=True)

    _output_results(final_specs, output)

def subsume_specs(specs):
    final_specs = []
    for s in specs:
        add = True
        for t in specs:
            if (t.rule.context.issubset(s.rule.context) or t.rule.response.issubset(s.rule.response)):
                if (s != t):
                    add = False

        print(add)
        if add:
            final_specs.append(s)

    return final_specs

def filter_specs(specs):
    """Filter specifications to reduce noise

    Filtering criteria
      - Only retain the highest local confidence for each response
      - Tie break with merged support (only support guaranteed to exist for all specs)
      - Remove rules with confidence < 0.5
      - Remove rules where the same function appears on left side and right side
    """

    # Map from response to tuple to keep
    highest_local = dict()

    # Store the spec with highest local confidence for each response
    for s in specs:
        response = frozenset(s.rule.response)
        if response not in highest_local:
            highest_local[response] = s
        elif s.local_confidence > highest_local[response].local_confidence:
            highest_local[response] = s
        elif s.local_confidence == highest_local[response].local_confidence:
            if s.merged_support > highest_local[response].merged_support:
                highest_local[response] = s

    ret = []
    for k in highest_local.keys():
        ret.append(highest_local[k])

    # Filter out all remaining specs with local confidence <= 0.5
    ret = filter(lambda spec: spec.local_confidence > 0.5, ret)

    return ret


def _output_results(specs, sheetname):
    # rule, individual support, merged support

    ### OUTPUT RESULTS TO GOOGLE SPREADSHEET ####
    # TODO: Parameterize sheet name as database name
    SPREADSHEET_ID = '10z85XJsVPX51pKB4fe9YGaRms_hiBMKtC_4S-_xPOUo'
    spreadsheet = SpecsSpreadsheet('credentials.json', SPREADSHEET_ID)

    # Clear existing specs data
    spreadsheet.clear(sheetname + '!A2:G151')

    spreadsheet.bulk_add_specs(sheetname + '!A2:F101', specs[:150])

def include_rule(rule):
    blacklist = set([
        "strcpy",
        "seq_printf",
        "sprintf",
        "__mlog_printk",
        "strcat",
        "strlcpy",
        "init_timer_key",
        "IS_ERR",
        "GFS2_I",
        "OCFS2_I",
        "BTRFS_I",
        "INODE_CACHE",
        "GFS2_SB",
        "EXT4_SB",
        "btrfs_sb"
        ])

    graylist = set([
        "dev_err",
        "dquot_initialize",
        "ocfs2_inode_lock_full_nested",
        "warn_slowpath_null",
        "ocfs2_init_dinode_extent_tree",
        "fs_path_alloc",
        "btrfs_next_leaf",
        "ocfs2_lock_refcount_tree",
        "ocfs2_check_dir_for_entry",
        "ocfs2_prepare_dir_for_insert",
        "warn_slowpath_fmt",
        "ocfs2_init_dealloc_ctxt"
    ])

    # COMMENT out union to see blacklist only
    blacklist = blacklist.union(graylist)

    return not (rule.context.issubset(blacklist) or rule.response.issubset(blacklist))

def merge(rules, model, similarity_threshold):
    """Takes list of association rules and returns list of equivalence
    classes based on a similarity threshold.

    Equivalence classes are constructed via graph connected components.
    """
    G = nx.Graph()

    from scipy.special import comb
    num_combos = comb(len(rules), 2)
    print(num_combos)

    i = 0
    for rule1, rule2 in itertools.combinations(rules, 2):
        i += 1
        print((float(i) / num_combos) * 100)
        _merge_pair(rule1, rule2, model, G, similarity_threshold)

    return list(nx.connected_components(G))

def _merge_pair(rule1, rule2, model, G, threshold):
    """ Adds edges to G for each pair of rules that exceed similarity threshold.

    TODO: Document how this works
    """
    context_intersection = rule1.context.intersection(rule2.context)
    response_intersection = rule1.response.intersection(rule2.response)

    if context_intersection and response_intersection:
        return []

    # Special case, merge pair rules where synonyms are on both sides
    if not (context_intersection or response_intersection):
        if len(rule1.context) == 1 and len(rule2.context) == 1 and len(rule1.response) == 1 and len(rule2.response) == 1:
            try:
                context_sim = model.similarity(next(iter(rule1.context)), next(iter(rule2.context)))
                response_sim = model.similarity(next(iter(rule1.response)), next(iter(rule2.response)))
            except:
                return

            if (context_sim > threshold and response_sim > threshold):
                G.add_edge(rule1, rule2)
                G.add_edge(rule1, AssociationRule(rule1.context, rule2.response))
                G.add_edge(rule1, AssociationRule(rule2.context, rule1.response))

    if context_intersection:
        sm = synonym_mapping(rule1.response, rule2.response, model, threshold)
        rules_to_connect = [AssociationRule(context_intersection, x) for x in sm]
        for r1, r2 in itertools.combinations(rules_to_connect, 2):
            G.add_edge(r1, r2)
    elif response_intersection:
        sm = synonym_mapping(rule1.context, rule2.context, model, threshold)
        rules_to_connect = [AssociationRule(context_intersection, x) for x in sm]
        for r1, r2 in itertools.combinations(rules_to_connect, 2):
            G.add_edge(r1, r2)


def synonym_mapping(set1, set2, model, threshold):
    """
    Returns (set1 \cap set2) \cup (i,j) for all synonym pairs i, j in product of set1, set2
    E.g.
    A B C
    A D E

    Where {B, D} are synonyms and {C, E} are synonyms, returns:
    {A, E}, {A, D}, {A, C}, {A, B}

    This leads to cross-synonym mappings, which are probably not desirable.
    Needs refinement by returning which synonyms map with what.
    For example, we don't want to connect {A, B} with {A, E}...
    This is just a first pass and we hope it gets filtered out in mining right now.
    """
    assert threshold

    intersection = set1.intersection(set2)

    new_sets = set()
    for i, j in itertools.product(set1, set2):
        if i == j:
            new_sets.add(frozenset(intersection))

        try:
            sim = (model.similarity(i, j))
        except:
            sim = 0.0

        if (sim >= threshold):
            # Keep track of all synonyms pairs
            new_sets.add(frozenset(intersection.union([i])))
            new_sets.add(frozenset(intersection.union([j])))

    return new_sets

