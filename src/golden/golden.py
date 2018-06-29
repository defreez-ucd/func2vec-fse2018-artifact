import base64
import os
import StringIO
from io import BytesIO
from collections import namedtuple

import boto3
import flask
from gensim.models import KeyedVectors
import itertools
import matplotlib.pyplot as plt

import walker.metric
import walker.golden
import walker.data
import walker.visualization

import ehnfer.commands

app = flask.Flask(__name__)
app.config.from_object(__name__)

params = 'arxiv_vmlinux_length100_walks100_interproc1_window1_mincount5_dimensions300'
interprocedural_model_file = '/data/current.model'

# Local copy of s3://program2vec/fse/flat.vec
flat_model_file = '/data/flat.vec'
intraprocedural_model_file = '/home/daniel/tmp/intra.vec'

# Local copy of s3://program2vec/fse/bias2-0_removecross1_length100_numwalks100_interproc1_window1_mincount5_dim300.model
biased_model_file = '/home/daniel/tmp/bias2-remove1.vec'
#local_clusters_file = '/home/daniel/tmp/clusters.txt'
local_clusters_file = '/home/daniel/tmp/flat3500.clusters'

sound_handler_db = '/data/pcisound.sqlite'
fs_handler_db = '/home/daniel/tmp/5fs-unmerged.sqlite'

handler_db=sound_handler_db
lower_support_threshold = 3
similarity_threshold = 0.6

local_marked_file = '/data/marked_specs.txt'

GOLDEN_INPUT_ALL = walker.golden.GoldenInput(
    use_underscore=True,
    use_bootstrap=True,
    clusters_file='arxiv/clusters/{}_k3500.txt'.format(params),
    reviewed_files = ['aditya-relations.txt',
                      'cindy-relations.txt',
                      'aditya-dd-relations.txt',
                      'two-function-relations.txt',
                      'golden-set-review-or-something-relations.txt',
                       'sys-relations.txt',
                      '2018-02-19-relations.txt',
                      '2018-02-19-b-relations.txt',
                      'daniel-relations.txt',
    ]
)

GOLDEN_INPUT_NOBOOT = walker.golden.GoldenInput(
    use_underscore=True,
    use_bootstrap=False,
    clusters_file='arxiv/clusters/{}_k3500.txt'.format(params),
    reviewed_files = ['aditya-relations.txt',
                      'cindy-relations.txt',
                      'aditya-dd-relations.txt',
                      'two-function-relations.txt',
                      'golden-set-review-or-something-relations.txt',
                       'sys-relations.txt',
                      '2018-02-19-relations.txt',
                      '2018-02-19-b-relations.txt',
                      'daniel-relations.txt'
    ]
)

GOLDEN_INPUT_UNDERSCORES = walker.golden.GoldenInput(
    use_underscore=True,
    use_bootstrap=False,
    clusters_file='arxiv/clusters/{}_k3500.txt'.format(params),
    reviewed_files = [
       # 'aditya-relations.txt',
       #               'cindy-relations.txt',
       #               'aditya-dd-relations.txt',
       #               'two-function-relations.txt',
       #               'golden-set-review-or-something-relations.txt',
       #                'sys-relations.txt',
       #               '2018-02-19-relations.txt',
       #               '2018-02-19-b-relations.txt',
       #               'daniel-relations.txt'
    ]
)

GOLDEN_INPUT_MANUAL = walker.golden.GoldenInput(
    use_underscore=False,
    use_bootstrap=False,
    clusters_file='arxiv/clusters/{}_k3500.txt'.format(params),
    reviewed_files = [
        'aditya-relations.txt',
        'cindy-relations.txt',
        'aditya-dd-relations.txt',
        'two-function-relations.txt',
        'golden-set-review-or-something-relations.txt',
        'sys-relations.txt',
        '2018-02-19-relations.txt',
        '2018-02-19-b-relations.txt',
        'daniel-relations.txt'
    ]
)

FSCORE_GOLDEN=GOLDEN_INPUT_UNDERSCORES

@app.route('/')
def home():
    return flask.render_template('home.html')

@app.route('/names')
def names():
    # Hardcode S3 path to defined functions, for now.
    s3 = boto3.resource('s3')
    defined_functions_file = StringIO.StringIO()
    s3.Object('program2vec', 'inputs/definedfunctions.txt').download_fileobj(defined_functions_file)

    defined_functions_file.seek(0)
    defined_functions = walker.data.read_functions(defined_functions_file)

    ### DUPLICATION ####
    # This is duplicated from walker.commands.golden_names
    prefix_groups, suffix_groups = walker.golden.group_by_suffix(defined_functions)

    # Get a list of the most common function name prefixes for each file
    # keywords is a map from keyword to the list of files in which that is a top 3 keyword
    keywords = walker.golden.common_keywords(defined_functions)

    # Function must start with the prefix and be a keyword that is paired with the correct file
    # Threshold is the minimum number of function names that use the keyword
    keyword_filtered_suffix_groups = walker.golden.filter_groups_by_keyword(suffix_groups, keywords, [], threshold=10)

    # Filter out functions that have differing first directory paths
    path_filtered_suffix_groups = walker.golden.filter_groups_by_path(keyword_filtered_suffix_groups)

    underscore_wrappers = walker.golden.underscore_wrappers(defined_functions, double=True, single=True)

    ### END DUPLICATION ####

    output = []
    for g in path_filtered_suffix_groups:
        functions = [f.name for f in g.functions]
        output.append(" ".join(functions))

    for pair in underscore_wrappers:
        output.append(" ".join(pair))

    return flask.render_template('names.html', groups=output)

@app.route('/clusters')
def clusters():
    s3 = boto3.resource('s3')
    clusters_file = StringIO.StringIO()
    s3.Object('program2vec', GOLDEN_INPUT_MANUAL.clusters_file).download_fileobj(clusters_file)

    clusters_file.seek(0)
    clusters_lines = clusters_file.readlines()

    return flask.render_template('clusters.html',
                                 groups=clusters_lines
    )

@app.route('/roc')
def roc():
    figfile = BytesIO()

    golden_output_all = walker.golden.golden(GOLDEN_INPUT_ALL)
    golden_output_noboot = walker.golden.golden(GOLDEN_INPUT_NOBOOT)
    golden_output_underscores = walker.golden.golden(GOLDEN_INPUT_UNDERSCORES)
    golden_output_manual = walker.golden.golden(GOLDEN_INPUT_MANUAL)

    num_true = None
    num_false = None

    flat_model = KeyedVectors.load_word2vec_format(flat_model_file, binary=False)
    #interproc_model = KeyedVectors.load_word2vec_format(interprocedural_model_file, binary=False)
    #intra_model = KeyedVectors.load_word2vec_format(intraprocedural_model_file, binary=False)
    biased_model = KeyedVectors.load_word2vec_format(biased_model_file, binary=False)

    #all_scores, sims, true_done, false_done = walker.metric.multi_roc([interproc_model, intra_model, flat_model],
    #                                              golden_output_all,
    #                                              ["interprocedural", "intraprocedural", "flat"],
    #                                              num_true,
    #                                              num_false)

    #print(true_done, false_done)
    #print(len(sims))

    plt.savefig(figfile, format='png')
    plt.clf()
    figfile.seek(0)
    all_figdata_png = base64.b64encode(figfile.getvalue())

    #noboot_scores, sims, true_done, false_done = walker.metric.multi_roc([interproc_model, intra_model, flat_model],
    #                                              golden_output_noboot,
    #                                              ["interprocedural", "intraprocedural", "flat"],
    #                                              num_true,
    #                                              num_false)

    #print(true_done, false_done)
    #print(len(sims))

    plt.savefig(figfile, format='png')
    plt.clf()
    figfile.seek(0)
    noboot_figdata_png = base64.b64encode(figfile.getvalue())

    underscore_scores, sims, true_done, false_done, unique_functions = walker.metric.multi_roc([biased_model, flat_model],
                                                  golden_output_underscores,
                                                  ["path-based", "flat"],
                                                  num_true,
                                                  num_false)

    print(true_done, false_done, unique_functions)

    plt.savefig(figfile, format='png')
    plt.clf()
    figfile.seek(0)
    underscore_figdata_png = base64.b64encode(figfile.getvalue())


    manual_scores, sims, true_done, false_done, unique_functions = walker.metric.multi_roc([biased_model, flat_model],
                                                  golden_output_manual,
                                                  ["path-based", "flat"],
                                                  num_true,
                                                  num_false)

    print(true_done, false_done, unique_functions)

    plt.savefig(figfile, format='png')
    plt.clf()
    figfile.seek(0)
    manual_figdata_png = base64.b64encode(figfile.getvalue())

    return flask.render_template('roc.html',
                                 all_plot=None,
                                 noboot_plot=None,
                                 underscore_plot=underscore_figdata_png,
                                 manual_plot=manual_figdata_png,
                                 all_scores=[],
                                 noboot_scores=[],
                                 underscore_scores=underscore_scores,
                                 manual_scores=manual_scores,
                                 similarities=[],
    )

@app.route('/golden')
def golden():
    # These should be actual inputs

    golden_output = walker.golden.golden(GOLDEN_INPUT_MANUAL)
    must_equiv_classes = golden_output.must_equiv_classes

    output = StringIO.StringIO()
    largest_class = 0
    for c in must_equiv_classes:
        if len(c) > largest_class:
            largest_class = len(c)
        output.write("{}<br><br>\n".format(" ".join([x.name for x in c])))
    output.seek(0)

    num_classes = len(must_equiv_classes)
    print("Largest class,", largest_class)

    return flask.render_template('golden.html',
                                 groups=output,
                                 params=params,
                                 true_relations=golden_output.num_true_relations,
                                 false_relations=golden_output.num_false_relations,
                                 num_classes=num_classes,
                                 use_underscore_names=GOLDEN_INPUT_ALL.use_underscore,
                                 use_bootstrap=GOLDEN_INPUT_ALL.use_bootstrap
    )

@app.route('/specs', methods=['GET'])
def specs():
    specs = ehnfer.commands.mine(db_file=handler_db,
                        support_threshold=lower_support_threshold,
                        similarity_threshold=similarity_threshold,
                        model_file=interprocedural_model_file,
                        num_epochs=10)

    # Read in the marked file
    judgements = dict()
    with open(local_marked_file, 'r') as f:
        lines= f.readlines()
    for l in lines:
        judgement, context, response = l.strip().split()
        if judgement == "T":
            judgement = True
        elif judgement == "?":
            judgement = None
        else:
            judgement = False
        judgements[(context, response)] = judgement

    # Commented sorts by support
    # Uncommented sorts by confidence
    #specs.sort(key=lambda x: x.confidence, reverse=True)

    rank = 1
    for s in specs:
        for r in s.rules:
            r.rank = rank
            rank += 1

#    sorted_judgements = []
#    for spec in specs:
#        # Calculate average precision of entire set, leaving gaps
#        for rule in spec.rules:
#            if (rule.item1.name, rule.item2.name) in judgements:
#                j = judgements[(rule.item1.name, rule.item2.name)]
#                rule.judgement = j
#                sorted_judgements.append(j)

    # Parse pairs, keeping the last in the file
    #if 'csv' in flask.request.args:
    #    return _specs_csv(specs)

#    avgprecision = walker.metric.avgprecision(sorted_judgements)

    return flask.render_template('specs.html', specs=specs, avgprecision=0.0)


def _specs_csv(specs):
    import csv

    si = StringIO.StringIO()
    cw = csv.writer(si)

    for s in specs:
        for r in s.rules:
            cw.writerow([r.item1, r.item2, s.support, r.support])

    output = flask.make_response(si.getvalue())
    output.headers["Content-Disposition"] = "attachment; filename=export.csv"
    output.headers["Content-type"] = "text/csv"

    return output

@app.route('/specs', methods=['POST'])
def mark_spec():
    # Parse the button ID
    button_id = flask.request.form['button']
    judgement, context, response = button_id.split('|')

    # Open the marked file for append and write out the pair
    f = open(local_marked_file, 'a')
    f.write("{} {} {}\n".format(judgement, context, response))
    f.close()

    return "Foo"


@app.route('/new')
def new_metric():
    #model = KeyedVectors.load_word2vec_format(local_model_file, binary=False)

    # Load the clusters
    with open(local_clusters_file, 'r') as f:
        clusters = [set(x.strip().split()) for x in f.readlines()]

    metric = walker.golden.new_metric(GOLDEN_INPUT, clusters)

    precision = float(metric.true_positives) / (metric.true_positives + metric.false_positives)
    recall = float(metric.true_positives) / (metric.true_positives + metric.false_negatives)
    f1 = 2 * precision * recall / (precision + recall)

    return flask.render_template('new.html',
                                 metric=metric,
                                 precision=precision,
                                 recall=recall,
                                 f1=f1,
    )

# TODO!: Print list of golden cluster functions that are not in the model (filtered out)
@app.route('/fscore')
def fscore():
    # Load the clusters
    with open(local_clusters_file, 'r') as f:
        clusters = [set(x.strip().split()) for x in f.readlines()]

    golden_output = walker.golden.golden(FSCORE_GOLDEN)
    must_equiv_classes = golden_output.must_equiv_classes

    # function_classes is just a reformatting of golden set into dict
    function_classes = dict()
    golden_set_size = 0
    for i in xrange(len(must_equiv_classes)):
        must_equiv_class = must_equiv_classes[i]
        for f in must_equiv_class:
            golden_set_size += 1
            function_classes[f.name] = i

    # Create a results class...lol
    results = walker.data.purity(function_classes, clusters)

    f_score = results[6][0][0]
    goldens = results[7]
    avg_precision = results[8][0][0]
    avg_recall = results[9][0][0]

    num_classes = len(goldens)

    # Go through each function in each golden
    # Each function will be a named tuple with the function name and a color to apply
    # Make two lists: functions only in beginning, functions that are not only in the beginning

    return flask.render_template('show_entries.html',
                                 groups=goldens,
                                 f_score=f_score,
#                                 total_golden_size=golden_set_size,
                                 avg_precision=avg_precision,
                                 avg_recall=avg_recall,
                                 params=params,
                                 true_relations=golden_output.num_true_relations,
                                 false_relations=golden_output.num_false_relations,
                                 num_classes=num_classes,
                                 use_underscore_names=GOLDEN_INPUT_MANUAL.use_underscore,
                                 use_bootstrap=GOLDEN_INPUT_MANUAL.use_bootstrap
    )


if __name__ == "__main__":
    app.run(host='0.0.0.0')
