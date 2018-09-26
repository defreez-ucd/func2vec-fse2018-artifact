import tempfile
import os
import subprocess

from association_rule import AssociationRule

class Eclat(object):
    def __init__(self):
        self.rules = frozenset()
        self.support_map = dict()

    def mine(self, sentences_file, support_threshold):
        rules = set()
        tmp_name_mined = "%s/%s" % (tempfile.gettempdir(), next(tempfile._get_candidate_names()))
        print("ECLAT RESULTS: %s" % tmp_name_mined)

        # Run eclat
        support_param = "-s-%s" % support_threshold
        script_dir = os.path.dirname(os.path.realpath(__file__))
        subprocess.call(
            ['/program2vec/lib/eclat/eclat/src/eclat',
             '-tc', support_param,
             '-v %a',
             sentences_file,
             tmp_name_mined])

        # Read in the frequent itemsets
        with open(tmp_name_mined, "r") as f:
            raw_mined = f.readlines()
            raw_mined = [x.rstrip() for x in raw_mined]

        for t in raw_mined:
            items = t.split()
            rule_support = int(items.pop())

            context_functions = set()
            response_functions = set()
            for item in items:
                prefix, item_name = item.split("|")
                if prefix == "PRE":
                    context_functions.add(item_name)
                else:
                    response_functions.add(item_name)

            if len(context_functions) != 0 and len(response_functions) != 0:
                rule = AssociationRule(
                    context=context_functions,
                    response=response_functions
                )
                rules.add(rule)
                self.support_map[rule] = rule_support

        self.rules = frozenset(rules)


    def get_rules(self):
        return self.rules

    def get_support_for_rule(self, rule):
        if rule in self.support_map:
            return self.support_map[rule]
        else:
            return None
