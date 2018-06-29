import boto3
import botocore

class JobRunner():
    def __init__(self):
        self.batch = boto3.client('batch', region_name='us-west-2')
        self.s3= boto3.client('s3')

        # Map from a config fragment (CompileConfig, WalkConfig, TrainConfig, ClusterConfig)
        # to a running job id. Used for dependencies and to prevent duplicated work.
        self.running_jobs = dict()

    def submit(self, aws_config):
        """
        :param aws_config: An AwsConfig object
        """
        run_config = aws_config.run_config

        # Compile bitcode
        compile_config = aws_config.compile_config
        compile_config_idx = (compile_config,)
        job_name = "compile_{}".format(run_config.config_name)
        if compile_config_idx not in self.running_jobs:
            if not self.s3_object_exists(aws_config.bitcode_file):
                linux_bitcode_vars = [
                    {
                        'name': 'INPUT_LINUX_CONFIG_S3_URL',
                        'value': compile_config.linux_config
                    },
                    {
                        'name': 'OUTPUT_38_BITCODE_S3_DIRECTORY',
                        'value': compile_config.linux_bitcode_dir
                    },
                    {
                        'name': 'OBJECT_PATHS',
                        'value': compile_config.object_paths
                    }
                ]
                print("Submitting {}".format(job_name))
                response = self.__submit_job(job_name=job_name, job_definition='linux_bitcode', env_vars=linux_bitcode_vars)
                self.running_jobs[compile_config_idx] = response["jobId"]
            else:
                print("compile: S3 cache found for {}".format(job_name))
        else:
            print("Running job found for {}".format(job_name))

        # Run walks
        walk_config = run_config.walk_config
        walk_config_idx = (compile_config, walk_config)
        job_name = "walk_{}_{}".format(run_config.config_name, walk_config.name)
        if walk_config_idx not in self.running_jobs:
            if not self.s3_object_exists(aws_config.walks_output):
                walk_vars = [
                    {
                        'name': 'FUNC2VEC_BITCODE_FILE_S3_URL',
                        'value': aws_config.bitcode_file
                    },
                    {
                        'name': 'FUNC2VEC_WALKS_OUTPUT_S3_URL',
                        'value': aws_config.walks_output
                    },
                    {
                        'name': 'FUNC2VEC_NUMWALKS',
                        'value': walk_config.num_walks
                    },
                    {
                        'name': 'FUNC2VEC_WALKLENGTH',
                        'value': walk_config.walk_length
                    },
                    {
                        'name': 'FUNC2VEC_INTERPROCEDURAL',
                        'value': walk_config.interprocedural
                    },
                    {
                        'name': 'REMOVE_CROSS_FOLDER',
                        'value': walk_config.remove_cross_folder
                    },
                    {
                        'name': 'BIAS_CONSTANT',
                        'value': walk_config.bias_constant
                    },
                ]

                print("Submitting walk job {}".format(job_name))
                if compile_config_idx in self.running_jobs:
                    print("Found running compile dependency for {}".format(job_name))
                    response = self.__submit_job(job_name=job_name, job_definition='func2vec_walk_bitcode',
                                                 env_vars=walk_vars,
                                                 dependencies=[self.running_jobs[compile_config_idx]])
                else:
                    response = self.__submit_job(job_name=job_name, job_definition='func2vec_walk_bitcode', env_vars=walk_vars)
                self.running_jobs[walk_config_idx] = response["jobId"]
            else:
                print("walk: S3 cache found for walk job {}".format(job_name))


        # Train model
        train_config = run_config.train_config
        train_config_idx = (compile_config, walk_config, train_config)
        job_name = "train_{}_{}_{}".format(run_config.config_name, walk_config.name, train_config.name)
        if train_config_idx not in self.running_jobs:
            if not self.s3_object_exists(aws_config.model_file):
                model_vars = [
                    {
                        'name': 'WALKS_INPUT_S3_URL',
                        'value': aws_config.walks_output
                    },
                    {
                        'name': 'MODEL_OUTPUT_S3_URL',
                        'value': aws_config.model_file
                    },
                    {
                        'name': 'WINDOW',
                        'value': train_config.window_size
                    },
                    {
                        'name': 'MINCOUNT',
                        'value': train_config.min_count
                    },
                    {
                        'name': 'DIMENSIONS',
                        'value': train_config.dimensions
                    },
                ]

                print("Submitting train job {}".format(job_name))
                if walk_config_idx in self.running_jobs:
                    print("Found running walk dependency for {}".format(job_name))
                    response = self.__submit_job(job_name=job_name, job_definition='func2vec_model_walks',
                                                 env_vars=model_vars,
                                                 dependencies=[self.running_jobs[walk_config_idx]])
                else:
                    response = self.__submit_job(job_name=job_name, job_definition='func2vec_model_walks',
                                                 env_vars=model_vars)
                self.running_jobs[train_config_idx] = response["jobId"]
            else:
                print("train: S3 cache found for train job {}".format(job_name))

        # Cluster
        cluster_config = run_config.cluster_config
        cluster_config_idx = (compile_config, walk_config, train_config, cluster_config)
        job_name = "cluster_{}_{}_{}_{}".format(run_config.config_name, walk_config.name, train_config.name, cluster_config.name)
        if cluster_config_idx not in self.running_jobs:
            if not self.s3_object_exists(aws_config.clusters_file):
                cluster_vars = [
                    {
                        'name': 'INPUT_MODEL_S3_URL',
                        'value': aws_config.model_file
                    },
                    {
                        'name': 'OUTPUT_CLUSTERS_S3_URL',
                        'value': aws_config.clusters_file
                    },
                    {
                        'name': 'NUMCLUSTERS',
                        'value': cluster_config.num_clusters
                    }
                ]

                print("Submitting cluster job {}".format(job_name))
                if train_config_idx in self.running_jobs:
                    print("Found running train dependency for {}".format(job_name))
                    response = self.__submit_job(job_name=job_name, job_definition='kmeans',
                                                 env_vars=cluster_vars,
                                                 dependencies=[self.running_jobs[train_config_idx]])
                else:
                    response = self.__submit_job(job_name=job_name, job_definition='kmeans',
                                                 env_vars=cluster_vars)
                self.running_jobs[cluster_config_idx] = response["jobId"]
            else:
                print("cluster: S3 cache found for {}".format(job_name))

        # Data purity numbers, no configuration
        job_name = "purity_{}_{}_{}_{}".format(run_config.config_name, walk_config.name, train_config.name, cluster_config.name)
        # Hack, because folders aren't really things in S3
        if not self.s3_object_exists(aws_config.purity_folder + "/f_score.csv"):
            purity_vars = [
               {
                   'name': 'INPUT_MODEL_S3_URL',
                   'value': aws_config.model_file
               },
               {
                   'name': 'INPUT_CLUSTERS_S3_URL',
                   'value': aws_config.clusters_file
               },
               {
                   'name': 'INPUT_GOLDEN_S3_URL',
                   'value': aws_config.golden_file
               },
               {
                   'name': 'OUTPUT_S3_FOLDER',
                   'value':aws_config.purity_folder
               },
            ]
            print("Submitting data purity job {}".format(job_name))
            if cluster_config_idx in self.running_jobs:
                print("Found running cluster dependency for {}".format(job_name))
                response = self.__submit_job(job_name=job_name, job_definition='data_purity',
                                                env_vars=purity_vars,
                                                dependencies=[self.running_jobs[train_config_idx]])
            else:
                response = self.__submit_job(job_name=job_name, job_definition='data_purity',
                                                env_vars=purity_vars)
        else:
            print("purity: S3 cache found for {}".format(job_name))

        print("----------")

    def terminate_all(self):
        for status in ['SUBMITTED', 'PENDING', 'RUNNABLE', 'STARTING', 'RUNNING']:
            joblist = self.batch.list_jobs(jobQueue='func2vec', jobStatus=status)["jobSummaryList"]
            for job in joblist:
                print(job)
                print("Terminating {}".format(job["jobId"]))
                self.batch.terminate_job(jobId=job["jobId"], reason="Because I can.")

    def __submit_job(self, job_name=None, job_definition=None, env_vars=[], dependencies=[],):
        """
        Runs an AWS batch job.

        :param job_name: The name to assign the job
        :param job_definition: The type of job to run
        :param env_dict: List of dictionaries with 'name' and 'value' keys.
        :param dependencies: List of job ids on which this job depends (sequential)
        :return: The AWS response
        """
        assert job_name and job_definition

        response = self.batch.submit_job(
            jobName=job_name,
            jobQueue='func2vec',
            jobDefinition=job_definition,
            containerOverrides={'environment': env_vars},
            dependsOn=[{'jobId': x} for x in dependencies],
        )
        return response

    def s3_object_exists(self, s3_path):
        s3_path = s3_path[5:]
        s3_components = s3_path.split('/')
        bucket = s3_components[0]
        s3_key = '/'.join(s3_components[1:])

        try:
            self.s3.head_object(Bucket=bucket, Key=s3_key)
            return True
        except botocore.exceptions.ClientError as e:
            if not e.response['Error']['Code'] == "404":
                raise
            else:
                return False
