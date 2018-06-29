import itertools

class Factory():
    walk_length = ['100', '300']
    window_size = ['1']
    min_count = ['5']
    num_walks = ['100']
    num_clusters = ['3500']
    interprocedural = ['1']
    remove_cross_folder = ['0', '1']
    bias_constant = ['1.0', '1.5', '2.0']
    dimensions = ['100']

    @staticmethod
    def get_configs(config_name):
        all_parameters = [Factory.walk_length, Factory.window_size, Factory.min_count,
                          Factory.num_walks, Factory.num_clusters, Factory.interprocedural,
                          Factory.remove_cross_folder, Factory.bias_constant, Factory.dimensions]

        configs = []
        for c in itertools.product(*all_parameters):
            walk_config = RunConfig.WalkConfig()
            walk_config.walk_length = c[0]
            walk_config.num_walks = c[3]
            walk_config.interprocedural = c[5]
            walk_config.remove_cross_folder = c[6]
            walk_config.bias_constant = c[7]

            train_config = RunConfig.TrainConfig()
            train_config.window_size = c[1]
            train_config.min_count = c[2]
            train_config.dimensions = c[8]

            cluster_config = RunConfig.ClusterConfig()
            cluster_config.num_clusters = c[4]

            this_config = RunConfig()
            this_config.config_name = config_name
            this_config.walk_config = walk_config
            this_config.train_config = train_config
            this_config.cluster_config = cluster_config
            configs.append(this_config)

        return configs

class RunConfig:
    class WalkConfig:
        def __init__(self):
            self.walk_length = None
            self.num_walks = None
            self.interprocedural = None
            self.remove_cross_folder = None
            self.bias_constant = None

        def __eq__(self, other):
            equal = True
            equal &= self.walk_length == other.walk_length
            equal &= self.num_walks == other.num_walks
            equal &= self.interprocedural == other.interprocedural
            equal &= self.remove_cross_folder == other.remove_cross_folder
            equal &= self.bias_constant == other.bias_constant
            return equal

        def __hash__(self):
            return hash((self.walk_length, self.num_walks, self.interprocedural, self.remove_cross_folder, self.bias_constant))

        @property
        def name(self):
            bias_nodot = self.bias_constant.replace('.', '-')
            return "bias{}_removecross{}_length{}_numwalks{}_interproc{}".format(
                bias_nodot, self.remove_cross_folder,
                self.walk_length, self.num_walks, self.interprocedural)

    class TrainConfig:
        def __init__(self):
            self.window_size = None
            self.min_count = None
            self.dimensions = None

        @property
        def name(self):
            return "window{}_mincount{}_dim{}".format(self.window_size, self.min_count, self.dimensions)

        def __equal__(self, other):
            equal = True
            equal &= self.window_size == other.window_size
            equal &= self.min_count == other.min_count
            equal &= self.dimensions == other.dimensions

        def __hash__(self):
            return hash((self.window_size, self.min_count, self.dimensions))

    class ClusterConfig:
        def __init__(self):
            self.num_clusters = None

        @property
        def name(self):
            return "k{}".format(self.num_clusters)

        def __eq__(self, other):
            return self.num_clusters == other.num_clusters

        def __hash__(self):
            return hash((self.num_clusters))

    def __init__(self):
        """Each stage writes something back to S3
        """
        self.config_name = None
        self.compile_config = None
        self.walk_config = None
        self.train_config = None
        self.cluster_config = None

class AwsConfig:
    class CompileConfig:
        def __init__(self):
            self.linux_config = None
            self.linux_bitcode_dir = None
            self.object_paths = None

        def __eq__(self, other):
            equal = True
            equal &= self.linux_config == other.linux_config
            equal &= self.linux_bitcode_dir == other.linux_bitcode_dir
            equal &= self.object_paths == other.object_paths
            return equal

        def __hash__(self):
            return hash((self.linux_config, self.linux_bitcode_dir, self.object_paths))


    """This controls where the file are stored in S3 (and therefore what needs to be re-run)
    """
    def __init__(self, run_config):
        self.compile_config = self.CompileConfig()
        self.compile_config.linux_config = 's3://rubio-students/daniel/linux-configs/default.config'
        # This is a directory, not an object, because this job supports extracting multiple object files
        self.compile_config.linux_bitcode_dir = 's3://program2vec/arxiv/bitcode'
        self.compile_config.object_paths = 'vmlinux.o'

        self.run_config = run_config

        # Walks
        self.bitcode_file = 's3://program2vec/arxiv/bitcode/vmlinux.o.bc'
        self.walks_output = 's3://program2vec/biased/walks/{}_{}.walks'.format(run_config.config_name, run_config.walk_config.name)

        # Model
        self.model_file = 's3://program2vec/biased/models/{}_{}.model'.format(
            run_config.walk_config.name, run_config.train_config.name)

        # Clustering
        self.clusters_file = 's3://program2vec/biased/clusters/{}_{}_{}.clusters'.format(
            run_config.walk_config.name, run_config.train_config.name, run_config.cluster_config.name)

        # Data purity
        self.golden_file = 's3://rubio-students/daniel/inputs/golden.txt'
        self.purity_folder = 's3://program2vec/biased/data/purity/{}_{}_{}'.format(
            run_config.walk_config.name, run_config.train_config.name, run_config.cluster_config.tname
        )
