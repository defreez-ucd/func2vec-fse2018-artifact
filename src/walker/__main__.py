import argparse
import commands
import logging

def main():
    command, command_kwargs = parse_args()
    command(**command_kwargs)


def parse_args(tsne=commands.tsne):
    DEFAULT_WALKS = 100
    DEFAULT_LENGTH = 200
    DEFAULT_PERPLEXITY = 30
    DEFAULT_WINDOW = 1
    DEFAULT_MINCOUNT = 5
    DEFAULT_VECTORSIZE = 100

    BITCODE_HELP = 'Path to bitcode file'
    OUTPUT_HELP = 'Path to output file'
    MODEL_HELP = 'Path to model file'
    PARSE_ERROR = 'There is a bug in the argument parser'

    command = None

    model_parsers = []

    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--getgraph', type=str, help='Path to getgraph binary')
    parser.add_argument('--errorcodes', type=str, help='Path to error codes file')
    subparsers = parser.add_subparsers(title='commands', dest='command')

    parser_walk = subparsers.add_parser('walk', help='Walk a bitcode file.')
    parser_walk.add_argument('--bitcode', type=str, help=BITCODE_HELP, required=True)
    parser_walk.add_argument('--output', type=str, help=OUTPUT_HELP)
    parser_walk.add_argument('--length', type=int, default=DEFAULT_LENGTH, help='Maximum path length')
    parser_walk.add_argument('--walks', type=int, default=DEFAULT_WALKS, help='Number of walks per label')
    parser_walk.add_argument('--debug', action='store_true', help='Print line numbers')
    parser_walk.add_argument('--remove', nargs='+', help='Space delimited list of labels to remove')
    parser_walk.add_argument('--interprocedural', type=int, choices=[0,1], default=1, help='Context sensitivite walk')
    parser_walk.add_argument('--enterexit', action='store_true', help='Function enter exit labels during walk')
    parser_walk.add_argument('--flat', action='store_true', help='No paths')
    parser_walk.add_argument('--bias', type=float, help="Bias constant (1.0 is unbiased)", default=1.0)
    parser_walk.add_argument('--distances', type=str, default=False, help='Write stack distances to this file.')
    parser_walk.add_argument('--remove-cross-folder', action='store_true', help='Remove cross-folder edges from points-to analysis')

    parser_pairsim = subparsers.add_parser('pairsim', help='Similarity of a pair of labels in a model')
    model_parsers.append(parser_pairsim)
    parser_pairsim.add_argument('--label1', help='First label (function name probably)', required=True)
    parser_pairsim.add_argument('--label2', help='Second label (function name probably)', required=True)

    parser_train = subparsers.add_parser('train', help='Train a model.')
    train = parser_train.add_mutually_exclusive_group(required=True)
    train.add_argument('--input', type=str, help='Read in walks/paths from a file')
    parser_train.add_argument('--output', type=str, help='Where to save the word2vec model', required=True)
    parser_train.add_argument('--length', type=int, default=DEFAULT_LENGTH, help='Maximum path length')
    parser_train.add_argument('--walks', type=int, default=DEFAULT_WALKS, help='Number of walks per label')
    parser_train.add_argument('--window', type=int, default=DEFAULT_WINDOW, help='Window size')
    parser_train.add_argument('--mincount', type=int, default=DEFAULT_MINCOUNT, help='Mincount')
    parser_train.add_argument('--size', type=int, default=DEFAULT_VECTORSIZE, help='Embedding dimensions')
    parser_train.add_argument('--skipgram', type=int, default=1, choices=[0,1], help='1 for skip-gram, 0 for CBOW.')
    parser_train.add_argument('--softmax', type=int, default=0, choices=[0,1], help='1 for hierarchical softmax, 0 for negative sampling.')
    parser_train.add_argument('--remove', nargs='+', help='Space delimited list of labels to remove')

    parser_tsne = subparsers.add_parser('tsne', help='Visualize a trained model with t-SNE.')
    parser_tsne.add_argument('--output', type=str, help=OUTPUT_HELP)
    parser_tsne.add_argument('--functionclasses', type=str, help='Input file of function names and class labels',
                             required=True)
    parser_tsne.add_argument('--perplexity', type=int, choices=range(5, 51), default=DEFAULT_PERPLEXITY)
    model_parsers.append(parser_tsne)

    parser_explore = subparsers.add_parser('explore', help='Explore the data')
    subparsers_explore = parser_explore.add_subparsers(title='subcommand', dest='subcommand')
    parser_explore_simhist = subparsers_explore.add_parser('simhist', help='Histograms of word vector similarities')
    model_parsers.append(parser_explore_simhist)
    parser_explore_simhist.add_argument('--model2', type=str, help="Second model for comparison")

    parser_explore_plotwalks = subparsers_explore.add_parser('plotwalks', help='Plot characteristics of random walks.')
    parser_explore_plotwalks.add_argument('--walksfile', type=str, help='File containing walks', required=True)
    parser_explore_plotwalks.add_argument('--label-o, type=output', dest='label_output_file', type=str,
                                  help='Where to save the label distribution plot')
    parser_explore_plotwalks.add_argument('--length-output', dest='length_output_file', type=str,
                                  help='Where to save the length plot')
    parser_explore_graphviz = subparsers_explore.add_parser('graphviz', help='Dump the input graph .dot file.')
    parser_explore_graphviz.add_argument('--bitcode', type=str, help=BITCODE_HELP, required=True)
    parser_explore_graphviz.add_argument('--output', type=str, help=OUTPUT_HELP)

    parser_explore_graphcharac = subparsers_explore.add_parser('graphcharac', help='Dump plots about graph characteristics to file.')
    parser_explore_graphcharac.add_argument('--bitcode', type=str, help=BITCODE_HELP, required=True)
    parser_explore_graphcharac.add_argument('--outputprefix', type=str, help="Prefix of output files", required=True)
 
    parser_cluster = subparsers.add_parser('cluster', help='Clustering subcommands')
    subparsers_cluster = parser_cluster.add_subparsers(title='subcommand', dest='subcommand')
    parser_kmeans = subparsers_cluster.add_parser('kmeans', help='Perform k-Means clustering on a model.')
    model_parsers.append(parser_kmeans)
    parser_kmeans.add_argument('--k', type=int, help="Number of clusters", required=True)
    parser_kmeans.add_argument('--output', type=str, help='Output file')
    parser_dbscan = subparsers_cluster.add_parser('dbscan', help='Perform dbscan clustering on a model.')
    model_parsers.append(parser_dbscan)
    parser_dbscan.add_argument('--eps', type=float, help="Epsilon", required=True)
    parser_dbscan.add_argument('--minsamples', type=int, help="Min samples", required=True)
    parser_dbscan.add_argument('--output', type=str, help='Output file')
    parser_random = subparsers_cluster.add_parser('random', help="Random dummy clusterer")
    model_parsers.append(parser_random)
    parser_random.add_argument('--k', type=int, help="Number of clusters", required=True)
    parser_random.add_argument('--output', type=str, help="Output file", required=True)

    parser_topic = subparsers.add_parser('topic', help='Topic modeling subcommands')
    subparsers_topic = parser_topic.add_subparsers(title='subcommand', dest='subcommand')
    parser_lda = subparsers_topic.add_parser('lda', help='Perform LDA topic modeling')
    parser_lda.add_argument('--walksfile', type=str, help='File containing walks', required=True)
    parser_lda.add_argument('--numtopics', type=str, help='Number of topics', required=True)
    parser_lda.add_argument('--output', type=str, help='Output file')

    parser_classify = subparsers.add_parser('classify', help='Classification subcommands')
    subparsers_classify = parser_classify.add_subparsers(title='subcommand', dest='subcommand')
    parser_logit = subparsers_classify.add_parser('logit', help='Logistic regression classification')
    model_parsers.append(parser_logit)
    parser_logit.add_argument('--classes', type=str, help="Input file containing function class pairs for training.",
                              required=True)
    parser_logit.add_argument('--average', type=str, choices=['micro', 'macro'], help="averaging method", required=True)
    parser_logit.add_argument('--zoomout', action='store_true', help="Set x and y axis limits to (0,1)")
    parser_logit.add_argument('--output', type=str, help='Save file for plot')


    parser_data = subparsers.add_parser('data', help='Data export commands')
    subparsers_data = parser_data.add_subparsers(title='subcommand', dest='subcommand')
    parser_data_full = subparsers_data.add_parser('full', help='Full data run from config file')
    parser_data_full.add_argument('--config', type=str, help='Path to config file', required=True)
    parser_data_full.add_argument('--task', type=str, help='Single task to run', required=False)
    parser_data_purity = subparsers_data.add_parser('purity', help='Raw precision/recall/purity data')
    parser_data_purity.add_argument('--output', type=str, help='Output directory where .csv files will be written to', required=True)
    parser_data_purity.add_argument('--clusters', type=str, help='Clusters file', required=True)
    parser_data_purity.add_argument('--reference', type=str, help='Reference sets file', required=True)
    model_parsers.append(parser_data_purity)
    parser_data_lpds = subparsers_data.add_parser('lpds', help='l-PDS counts for a bitcode file (nodes, rules, and labels)')
    parser_data_lpds.add_argument('--bitcode', type=str, help='Bitcode file', required=True)
    parser_data_lpds.add_argument('--output', type=str, help='Output file')

    parser_metric = subparsers.add_parser('metric', help='Cardinal metrics')
    subparsers_metric = parser_metric.add_subparsers(title='subcommand', dest='subcommand')
    parser_metric_avgprecision = subparsers_metric.add_parser('avgprecision')
    parser_metric_avgprecision.add_argument('--input', type=str, required=True)
    parser_metric_f1 = subparsers_metric.add_parser('f1', help='F1 cross-validation score')
    model_parsers.append(parser_metric_f1)
    parser_metric_f1.add_argument('--functionclasses', type=str, help='Input file of function names and class labels',
                                  required=True)
    parser_metric_f1.add_argument('--average', type=str, choices=['micro', 'macro'], help="averaging method", required=True)
    parser_metric_f1cluster = subparsers_metric.add_parser('f1cluster', help='F1 score for clusters')
    parser_metric_f1cluster.add_argument('--reference', type=str, help='Input file of function names and class labels',
                                  required=True)
    parser_metric_f1cluster.add_argument('--clusters', type=str, help="Clusters output file", required=True)
    parser_metric_roc = subparsers_metric.add_parser('roc', help='Sorted similarity AUC ROC')
    model_parsers.append(parser_metric_roc)
    parser_metric_roc.add_argument('--reference', type=str, help='Reference file (one group per line)', required=True)
    parser_metric_roc.add_argument('--png', type=str, help='Output path for ROC curve', required=True)

    parser_golden = subparsers.add_parser('golden', help='Golden set commands')
    subparsers_golden = parser_golden.add_subparsers(title='subcommand', dest='subcommand')
    parser_golden_names = subparsers_golden.add_parser('names', help='Output golden set using names heuristic')
    parser_golden_names.add_argument('--functions', type=str, help='Path to defined functions file', required=True)
    parser_golden_names.add_argument('--output', type=str, help='Output file', required=False)

    parser_aws = subparsers.add_parser('aws', help='Run jobs on aws.')
    subparsers_aws = parser_aws.add_subparsers(title='subcommand', dest='subcommand')
    parser_aws_endtoend = subparsers_aws.add_parser('endtoend', help='Run everything unless it is already in S3')
    parser_aws_terminate = subparsers_aws.add_parser('terminate', help='Terminate all')

    for p in model_parsers:
        p.add_argument('--model', type=str, help=MODEL_HELP, required=True)

    args = parser.parse_args()

    if args.debug:
        logger = logging.getLogger("func2vec")
        logger.setLevel(logging.DEBUG)

    command_kwargs = dict()

    if args.getgraph:
        command_kwargs["getgraph_binary"] = args.getgraph
    if args.errorcodes:
        command_kwargs["error_codes"] = args.errorcodes

    if args.command == "walk":
        command = commands.walk
        command_kwargs["bitcode_file"] = args.bitcode
        command_kwargs["output_file"] = args.output
        command_kwargs["length"] = args.length
        command_kwargs["walks"] = args.walks
        command_kwargs["remove"] = args.remove
        command_kwargs["interprocedural"] = args.interprocedural
        command_kwargs["enterexit"] = args.enterexit
        command_kwargs["flat"] = args.flat
        command_kwargs["bias_constant"] = args.bias
        command_kwargs["output_distances"] = args.distances
    elif args.command == "flatwalk":
        command = commands.flat_walk
        command_kwargs["bitcode_file"] = args.bitcode
        command_kwargs["output_file"] = args.output
    elif args.command == "train":
        command = commands.train
        command_kwargs["output_file"] = args.output
        command_kwargs["length"] = args.length
        command_kwargs["walks"] = args.walks
        command_kwargs["window"] = args.window
        command_kwargs["mincount"] = args.mincount
        command_kwargs["paths_file"] = args.input
        command_kwargs["size"] = args.size
        command_kwargs["skipgram"] = args.skipgram
        command_kwargs["hs"] = args.softmax
        command_kwargs["remove"] = args.remove
    elif args.command == "tsne":
        command = commands.tsne
        command_kwargs["model_file"] = args.model
        command_kwargs["output_file"] = args.output
        command_kwargs["functionclasses_file"] = args.functionclasses
        command_kwargs["perplexity"] = args.perplexity
    elif args.command == "plotwalks":
        command = commands.plotwalks
        command_kwargs["walks_file"] = args.walksfile
        command_kwargs["label_output_file"] = args.label_output_file
        command_kwargs["length_output_file"] = args.length_output_file
    elif args.command == "cluster":
        if args.subcommand == "kmeans":
            command = commands.kmeans
            command_kwargs["model_file"] = args.model
            command_kwargs["nclusters"] = args.k
            command_kwargs["output_file"] = args.output
        if args.subcommand == "random":
            command = commands.cluster_random
            command_kwargs["model_file"] = args.model
            command_kwargs["k"] = args.k
            command_kwargs["output_file"] = args.output
        elif args.subcommand == "dbscan":
            command = commands.dbscan
            command_kwargs["model_file"] = args.model
            command_kwargs["eps"] = args.eps
            command_kwargs["min_samples"] = args.minsamples
            command_kwargs["output_file"] = args.output
    elif args.command == "classify":
        if args.subcommand == "logit":
            command = commands.logit
            command_kwargs["model_file"] = args.model
            command_kwargs["classes_file"] = args.classes
            command_kwargs["average"] = args.average
            command_kwargs["zoomout"] = args.zoomout
            command_kwargs["output_file"] = args.output
    elif args.command == "topic":
        if args.subcommand == "lda":
            command = commands.lda
            command_kwargs["walks_file"] = args.walksfile
            command_kwargs["numtopics"] = args.numtopics
            command_kwargs["output_file"] = args.output
    elif args.command == "graphviz":
        command = commands.graphviz
        command_kwargs["bitcode_file"] = args.bitcode
        command_kwargs["output_file"] = args.output
    elif args.command == "graphcharac":
        command = commands.graphcharac
        command_kwargs["bitcode_file"] = args.bitcode
        command_kwargs["output_prefix"] = args.outputprefix
    elif args.command == "data":
        if args.subcommand == "full":
            command = commands.data_full
            command_kwargs["config_file"] = args.config
            command_kwargs["task_name"] = args.task
        elif args.subcommand == "purity":
            command = commands.data_purity
            command_kwargs["model_file"] = args.model
            command_kwargs["output_directory"] = args.output
            command_kwargs["clusters_file"] = args.clusters
            command_kwargs["reference_file"] = args.reference
        elif args.subcommand == "lpds":
            command = commands.data_lpds
            command_kwargs["bitcode_file"] = args.bitcode
            command_kwargs["output_file"] = args.output
    elif args.command == "metric":
        if args.subcommand == "f1":
            command = commands.metric_f1
            command_kwargs["model_file"] = args.model
            command_kwargs["functionclasses_file"] = args.functionclasses
            command_kwargs["average"] = args.average
        elif args.subcommand == "f1cluster":
            command = commands.metric_f1cluster
            command_kwargs["reference_file"] = args.reference
            command_kwargs["clusters_file"] =  args.clusters
        elif args.subcommand == "analogies":
            command = commands.metric_analogies
            command_kwargs["model_file"] = args.model
            command_kwargs["questions_file"] = args.questions
            command_kwargs["clusters_file"] = args.clusters
        elif args.subcommand == "silhouette":
            command = commands.metric_silhouette
            command_kwargs["model_file"] = args.model
            command_kwargs["k"] = args.k
        elif args.subcommand == "avgprecision":
            command = commands.metric_avgprecision
            command_kwargs["input"] = args.input
        elif args.subcommand == "sg":
            command = commands.metric_sg
            command_kwargs["model_file"] = args.model
            command_kwargs["word1"] = args.word1
        elif args.subcommand == "roc":
            command = commands.metric_roc
            command_kwargs["model_file"] = args.model
            command_kwargs["reference_file"] = args.reference
            command_kwargs["png_file"] = args.png
    elif args.command == "explore":
        if args.subcommand == "simhist":
            command = commands.simhist
            command_kwargs["model_file"] = args.model
            command_kwargs["model2_file"] = args.model2
        elif args.subcommand == "pairsim":
            command = commands.pairsim
            command_kwargs["model_file"] = args.model
            command_kwargs["label1"] = args.label1
            command_kwargs["label2"] = args.label2
    elif args.command == "golden":
        if args.subcommand == "names":
            command = commands.golden_names
            command_kwargs["functions_file"] = args.functions
            command_kwargs["output_file"] = args.output
    elif args.command == "aws":
        if args.subcommand == "endtoend":
            command = commands.aws_endtoend
        elif args.subcommand == "terminate":
            command = commands.aws_terminate

    if not command:
        raise Exception(PARSE_ERROR)

    return command, command_kwargs


if __name__ == "__main__":
    global RUN_FROM_CLI
    RUN_FROM_CLI=True
    logging.basicConfig(format='%(asctime)s : %(levelname)s : %(message)s', level=logging.INFO)
    main()
