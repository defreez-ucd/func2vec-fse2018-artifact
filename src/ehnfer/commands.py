from __future__ import print_function
import sys
import os
import itertools
import logging
from collections import defaultdict
import math
import scipy.misc

from ehnfer.handler_db import HandlerDb, Item, ItemType, TacticType, Spec
from gensim.models.keyedvectors import KeyedVectors

def mine(db_file=None, support_threshold=3, similarity_threshold=0.9, model_file=None, num_epochs=10, min_items=2, max_items=3):
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

    # Open the handler database and read handler information
    hdb = HandlerDb(db_file)

    logging.basicConfig(format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p', level=logging.INFO)

    # A map from an item to all of the handlers that have that item in their context
    import tempfile
    import subprocess

    # Mine three item closed sets using eclat
    tmp_name_tx = "%s/%s" % (tempfile.gettempdir(), next(tempfile._get_candidate_names()))
    tmp_name_mined = "%s/%s" % (tempfile.gettempdir(), next(tempfile._get_candidate_names()))

    # Call miner
    set_counts = defaultdict(set)

    # Write out sentences to tmp file
    # For each handler
    with open(tmp_name_tx, 'w') as f:
        for h in hdb.handlers.keys():
            # For each pair of items, one from context and one from response
            handler = hdb.handlers[h]

            # Print out handler sentence
            if len(handler.context) == 0 or len(handler.response) == 0:
                continue
            sentence = ["PRE|" + x.name for x in handler.context]
            sentence += (["POST|" + x.name for x in handler.response])
            f.write(" ".join(sentence))
            f.write("\n")
    print("SENTENCES AT:", tmp_name_tx)

    # Run eclat
    support_param = "-s-%s" % support_threshold
    max_item_param = "-n{}".format(max_items)
    min_item_param = "-m{}".format(min_items)
    script_dir = os.path.dirname(os.path.realpath(__file__))
    subprocess.call(
        ['/program2vec/lib/eclat/eclat/src/eclat', '-ts', support_param, max_item_param, min_item_param, '-v %a', tmp_name_tx, tmp_name_mined])
#    os.remove(tmp_name_tx)

    # Read in the frequent itemsets
    rules = set()
    with open(tmp_name_mined, "r") as f:
        raw_mined = f.readlines()
    os.remove(tmp_name_mined)
    raw_mined = [x.rstrip() for x in raw_mined]
    for t in raw_mined:
        items = t.split()
        rule_support = int(items.pop())

        context_functions = set()
        response_functions = set()
        for item in items:
            prefix, item_name = item.split("|")
            if item_name in blacklist:
                continue
            if prefix == "PRE":
                context_functions.add(item_name)
            else:
                response_functions.add(item_name)
        if len(context_functions) != 0 and len(response_functions) != 0:
            rules.add(AssociationRule(context=context_functions, response=response_functions, support=rule_support))

    # Convert to specs, but support is already done

    specs = [Spec(set([x])) for x in rules]
    print(support_threshold, scipy.misc.comb(len(specs), 2))

    print(len(specs))
    single_item_responses = defaultdict(set)
    for s in specs:
        for r in s.rules:
            if len(r.response) == 1:
                item = next(iter(r.response))
                single_item_responses[item].add(s)

    model = KeyedVectors.load_word2vec_format(model_file, binary=False)
    print(len(single_item_responses.keys()))

    # INSIGHT: We only need the raw handler list for merges, so calc. for rules on demand

    spec_sims = []
    for response1, response2 in itertools.combinations(single_item_responses, 2):
        specs1 = single_item_responses[response1]
        specs2 = single_item_responses[response2]

        for spec1, spec2 in itertools.product(specs1, specs2):
            spec_sims.append((spec1, spec2, spec1.similarity(spec2, model)))
    spec_sims.sort(key=lambda x: x[2], reverse=True)

    # Go through the spec pairs with sim above threshold
    # Merge them in order
    total_merges = 0
    for i in xrange(num_epochs):
        specs = set(specs)
        print("Starting epoch {} of {}".format(i+1, num_epochs))
        merged_this_epoch = 0
        done = 0

        new_sims = []
        for ss in spec_sims:
            spec1 = ss[0]
            spec2 = ss[1]
            sim = ss[2]
            print(sim)

            done += 1
            print(done, len(spec_sims))
            if sim >= similarity_threshold:
                if spec1 not in specs or spec2 not in specs:
                    continue

                # Add merged spec to list of all specs, and remove individual specs
                merged = spec1.merge(spec2, hdb)

                for s in specs:
                    new_sims.append((merged, s, merged.similarity(s, model)))

                specs.add(merged)
                specs.remove(spec1)
                specs.remove(spec2)

                # Recompute support for merged specifications
                merged_this_epoch += 1
            else:
                break

        spec_sims += new_sims
        spec_sims.sort(key=lambda x: x[2], reverse=True)
        specs = list(specs)
        specs.sort(key=lambda x: x.support, reverse=True)

        total_merges += merged_this_epoch
        print("{} merges".format(merged_this_epoch))
        if merged_this_epoch == 0:
            break

    specs.sort(key=lambda x: x.support, reverse=True)

    return specs

class Spec:
    def __init__(self, rules):
        """
        :param rules: A set of AssociationRules
        """
        self.rules = rules
        self.confidence = None

    def min_similarity(self, other, model):
        min_sim = 1.0
        for r1, r2 in itertools.product(self.rules, other.rules):
            sim = r1.similarity(r2, model)
            if sim < min_sim:
                min_sim = sim
        return min_sim

    def similarity(self, other, model):
        return self.min_similarity(other, model)

    def merge(self, other, hdb):
        new_rules = self.rules.union(other.rules)

        for rule in new_rules:
            for h in hdb.handlers.keys():
                handler = hdb.handlers[h]
                handler_context = set([x.name for x in handler.context])
                handler_response = set([x.name for x in handler.response])

                if rule.context.issubset(handler_context) and rule.response.issubset(handler_response):
                    rule.supporting_handlers.add(handler)

        # For now just take the union
        # Let's see what happens
        return Spec(new_rules)

    def __repr__(self):
        ret = "support: {}\n".format(self.support)
        ret += "\n".join([str(r) for r in self.rules])
        return ret

    @property
    def support(self):
        support = 0
        supporting_handlers = set()

        if len(self.rules) == 1:
            return next(iter(self.rules)).support

        for r in self.rules:
            # Merged specs must have supporting handlers populated for rules
            if len(self.rules) > 1:
                assert len(r.supporting_handlers) > 0
            for h in r.supporting_handlers:
                supporting_handlers.add(h)
        return len(supporting_handlers)

    @property
    def id(self):
        return id(self)


class AssociationRule:
    def __init__(self, context, response, support):
        self.context = context
        self.response = response
        self.__support = support
        self.judgement = None

        self.supporting_handlers = set()
        self.violating_handlers = set()

        self.rank = None

    @property
    def support(self):
        if len(self.supporting_handlers) == 0:
            return self.__support
        else:
            return len(self.supporting_handlers)

    def __repr__(self):
        return "{} -> {} s{}".format(self.context, self.response, self.support)

    def similarity(self, other, model):
        try:
            if self.context == other.context and len(self.response) == 1:
                return model.similarity(next(iter(self.response)), next(iter(other.response)))
            elif self.response == other.response and len(self.context) == 1:
                return model.similarity(next(iter(self.context)), next(iter(other.context)))
            else:
                return 0.0
        except:
            # Not in model
            return 0.0

    @property
    def id(self):
        return id(self)

