import random
import logging
import itertools
import os
import subprocess
from collections import defaultdict
import edgelist_pb2    # protobuf
import networkx as nx

class PushDown:
    class StackDistance:
        def __init__(self, calls, returns):
            self.calls = calls
            self.returns = returns

        def __str__(self):
            return "(calls: {}, returns: {})".format(self.calls, self.returns)

        @property
        def distance(self):
            #return self.calls + self.returns
            return self.calls

    def __init__(self, G, enterexit=False, interprocedural=True, bias_constant=1.0):
        """
        Creates a PDS-like-thing from a networkx graph. The only useful
        operation on an object of this type is to call random_walk or
        random_walk_all_labels.

        The return node is pushed onto the stack when a function call is
        followed. When a function call exit node is reached, a symbol is popped
        off the stack and that node is visited next.

        With the following types of edges
            - All edges must have "label" and "push" attributes, which can be None
            - "internal" edges have an out degree > 0 and push == None
            - "call" edges come from a node with an out degree > 0 and push != None
        Function exit nodes have an out degree of zero. The may_ret attribute of these
        nodes indicates the possible return sites.

        Leaf nodes are treated as popping the stack and going to the corresponding node

        :param G: A networkx graph
        :param enterexit: Print enter/exit words when entering/leaving a function
        :param interprocedural: Perform interprocedural walks
        :param bias_constant: min(0.5, 1 / bias_constant ** stack_distance)
        """
        self.G = G                                     # networkx graph
        self.stack = []                                # current stack during walk
        self.rand = random.Random()
        self.rand.seed(a=0)
        self.logger = logging.getLogger("func2vec")
        self.enterexit = enterexit                     # enter/exit padding labels
        self.interprocedural = interprocedural         # interprocedural walks?
        self.max_stack_distances = []                  # list of max stack distances encountered
        assert bias_constant != 0.0
        self.bias_constant = bias_constant             # 1.0 is unbiased

        # Map label to target node(s) for labeled edge
        self.label_to_nodes = defaultdict(list)
        for u, v, d in self.G.edges(data=True):
            if d["label"]:
                for id in d["label"]:
                    self.label_to_nodes[id].append((v, d["label"]))


    def random_walk_all_labels(self, path_length, walks_per_label, out=None):
        """
        Perform a random walk from each label (in random order).
        :param path_length: Length to truncate paths to
        :param walks_per_label: Number of random walks per label
        :param Where to write output (usually a file object)
        :return: walks - list of lists of labels
        """
        walks = []
        ids_to_walk = self.label_to_nodes.keys()

        for i in range(walks_per_label):
            self.logger.info("Starting walk %s of %s" % (i+1, walks_per_label))
            self.rand.shuffle(ids_to_walk)
            for id in ids_to_walk:
                self.logger.debug("Label is %s", self.G.graph["id_to_label"][id])
                walk = self.random_walk(path_length, start_label=id)[0]
                if len(walk) > 1:
                    if out:
                        out.write("%s\n" % ' '.join(walk))
                    else:
                        walks.append(walk)

        return walks

    def parallel_walk_all_labels(self, path_length, walks_per_label, out):
        """
        Perform a random walk from each label (in random order). Uses all CPUs.
        :param path_length: Length to truncate paths to
        :param walks_per_label: Number of random walks per label
        :param Where to write output (usually a file object)
        :return: walks - list of lists of labels
        """

        def task(data, result_q):
            walks = []
            for i in data:
                self.logger.info("Starting walk %s of %s" % (i+1, walks_per_label))
                task_ids_to_walk = list(self.label_to_nodes.keys())
                self.rand.shuffle(task_ids_to_walk)
                for id in task_ids_to_walk:
                    walk = self.random_walk(path_length, start_label=id)[0]
                    if len(walk) > 1:
                            walks.append(walk)
            result_q.put(walks)

        import multiprocessing
        import math
        data = range(walks_per_label)
        num_jobs = multiprocessing.cpu_count()
        chunksize = int(math.ceil(len(data) / float(num_jobs)))
        processes = []
        Q = multiprocessing.Queue()

        for i in range(num_jobs):
            start = chunksize * i
            end = chunksize * (i + 1)
            task_data = data[start:end]
            p = multiprocessing.Process(target=task, args=[task_data, Q])
            processes.append(p)
            p.start()

        # The order of jobs does not matter since each result is an independent walk of all labels
        # We just don't want to interleave the output from separate processes
        for i in range(num_jobs):
            walks_result = Q.get()
            for w in walks_result:
                out.write("%s\n" % ' '.join(w))

        for p in processes:
            p.join()

    def random_walk(self, path_length, start_label=None):
        """
        Peforms a single random walk
        :return: walk - list of labels encountered during walk
        """
        # Randomly sample a start label
        if not start_label:
            start_label = self.rand.choice(self.label_to_nodes.keys())
        start_node, start_labels = self.rand.choice(self.label_to_nodes[start_label])

        self.stack = []

        walk = [self.G.graph["id_to_label"][x].label for x in start_labels]
        next_node = start_node
        edges_visited = 1
        self.stack_distance = self.StackDistance(0, 0)
        self.max_stack_distances.append(0)
        while edges_visited < path_length:
            self.logger.debug(next_node)
            cur = next_node
            next_node = self.rand_transition(cur, walk)
            if not next_node:
                break
            edges_visited += 1

        return (walk, edges_visited)

    def rand_transition(self, node, walk):
        """
        Follows a random transition out of a node, and manipulates the stack accordingly.
        :param node: The current node (convert to member or use PDS-style stack to keep track of this)
        :return: (The node arrived at, true/false followed an edge)
        """
        neighbors = self.G.adj[node].keys()

        # Alter probability of following call edges for biased walk
        if "return_node" in self.G.node[node]:
            prob_enter_call = min(0.5, 1.0 / self.bias_constant ** self.stack_distance.distance)
            # Biased coin flip
            if random.random() < prob_enter_call:
                # Enter the call this walk by removing return node from possible targets
                neighbors.remove(self.G.node[node]['return_node'])
            else:
                # Skip the call this walk leaving only the return node in possible targets
                neighbors = [self.G.node[node]['return_node']]

        # Randomly select a neighbor
        # Avoid out_edges() because it is slow
        v = None
        if len(neighbors) > 0:
            # Choose a random neighbor
            v = self.rand.choice(neighbors)

        # Pop
        if not v:
            if self.enterexit:
                walk.append("F2V_EXITFN")
            if not self.interprocedural:
                return None
            elif len(self.stack) == 0:
                # Randomly select the entry point of a function that could be our caller
                # If there are no may_ret edges than this is the main function and we must stop
                if "may_ret" not in self.G.node[node]:
                    return None
                may_return_to = self.G.node[node]["may_ret"]
                chosen_return = self.rand.choice(may_return_to)
                self.stack_distance.returns += 1
                if self.stack_distance.distance > self.max_stack_distances[-1]:
                    self.max_stack_distances[-1] = self.stack_distance.distance
                return chosen_return
            elif len(self.stack) > 0:
                self.stack_distance.calls -= 1
                return self.stack.pop()

        u = node
        # There can be multiple edges (u, v) with different labels
        # Randomly select one of those edges to pull the label from
        data = random.choice(self.G[u][v])

        if data["label"]:
            walk += [self.G.graph["id_to_label"][x].label for x in data["label"]]

        # Internal
        if not data["push"]:
            return v

        # Call
        if self.G.out_degree(u) > 0 and data["push"]:
            if self.enterexit:
                walk.append("F2V_ENTERFN")
            self.stack.append(data["push"])
            self.stack_distance.calls += 1
            if self.stack_distance.distance > self.max_stack_distances[-1]:
                self.max_stack_distances[-1] = self.stack_distance.distance
            return v

        assert False

def generate_edgelist(bitcode_path, getgraph_binary, error_codes, remove_labels=None, remove_cross_folder=False):
    """
    Uses getgraph to generate an edgelist
    :param bitcode_path:
    :return: A list of edge tuples of the form (u, v, label, location)
    """
    if not os.path.isfile(bitcode_path):
        raise Exception('Unable to find bitcode file %s' % bitcode_path)

    # Try the current directory by default
    if not getgraph_binary:
        getgraph_binary = "./getgraph"
    if not os.path.isfile(getgraph_binary):
        raise Exception(
            'Unable to find getgraph at %s. You can use --getgraph to specify the location.' % getgraph_binary)

    getgraph_cmd = [getgraph_binary,
                    '--edgelist',
                    '--protobuf',
                    '--bitcode',
                    bitcode_path
    ]
    if error_codes:
        getgraph_cmd.append('--error-codes')
        getgraph_cmd.append(error_codes)
    if remove_cross_folder:
        getgraph_cmd.append('--remove-cross-folder')

    if error_codes:
        process = subprocess.Popen([getgraph_binary,
                                    '--edgelist',
                                    '--protobuf',
                                    '--bitcode',
                                    bitcode_path,
                                    '--error-codes',
                                    error_codes],
                                   stdout=subprocess.PIPE)
    else:
        process = subprocess.Popen([getgraph_binary,
                                    '--edgelist',
                                    '--protobuf',
                                    '--bitcode',
                                    bitcode_path],
                                   stdout=subprocess.PIPE)

    out, err = process.communicate()
    if err:
        raise Exception("Unable to communicate with getgraph.")

    protobuf_edgelist = edgelist_pb2.Edgelist()
    protobuf_edgelist.ParseFromString(out)

    return protobuf_edgelist


def create_graph_from_edgelist(protobuf_edgelist, remove_labels=None, interprocedural=True):
    """
    Reads the protobuf format of an edgelist output by getgraph. Several transformations are applied.
        - The main.0 node is removed
        - may_ret edges are removed, but the data is saved in the attributes of the exit node
        - Edges labeled "ret" are labeled with the function that is being returned from
        - Edges with the label "call" have that label removed
    :param edgelist: A list of edge tuples of the form (u, v, label)
    :return:
    """
    # Create the networkx graph
    G = nx.MultiDiGraph()
    id_to_label = dict(protobuf_edgelist.id_to_label)
    filtered_id_to_label = dict()
    if remove_labels:
        for k, v in id_to_label.iteritems():
            add = True
            for r in remove_labels:
                if v.label.startswith(r):
                    add = False
                    break
            if add:
                filtered_id_to_label[k] = v
    else:
        filtered_id_to_label = id_to_label
    # filtered_id_to_label = {k: v for k, v in id_to_label.iteritems() if v.label not in remove_labels}

    G.graph["id_to_label"] = filtered_id_to_label

    for e in protobuf_edgelist.edge:
        # Do not add main.0
        if (e.source == "main.0" or e.target == "main.0"):
            continue

        # Do not add ret edges, but store target
        # The "ret" edge is the only way that we identify the return site
        # We are going to be overwriting this with labeled edges, so we store
        # The attribute in the node. This could probably be cleared when we are done.
        if (e.label == "ret"):
            G.add_node(e.source)
            G.add_node(e.target)
            G.node[e.source]["return_node"] = e.target
            continue

        # Transform may_ret edges into a dictionary attribute
        # We do this because we do not want may_ret edges, but we need
        # to know the possible targets to jump to when the stack is empty
        if (e.label == "may_ret"):
            # G[][] is an edge
            # G.node[][] is an attribute of a node
            if "may_ret" in G.node[e.source]:
                G.node[e.source]["may_ret"].append(e.target)
            else:
                G.node[e.source]["may_ret"] = [e.target]
        else:
            # The push attribute indicates symbols that need to be pushed onto the stack
            # A node will either be a call site (outgoing call edges and a return node),
            # or an internal edge
            if e.label == "call":
                # Call dge
                G.add_edge(e.source, e.target, label=e.label, location=e.location, push=None)
            else:
                # Internal edge
                labels = [x for x in e.label_id if x in G.graph["id_to_label"].keys()]
                G.add_edge(e.source, e.target, label=labels, location=e.location, push=None)

    # Transform the graph
    # For every return edge, label it with the name of the function that is called
    if G.graph["id_to_label"]:
        next_id = max(id_to_label.iterkeys()) + 1
    else:
        next_id = 0
    funcname_to_id = dict()  # map from function name to id.
    for u, v, d in G.edges(data=True):
        if "label" in d and d["label"] == "call":
            assert "return_node" in G.node[u]
            funcname = v.split('.')[0]
            # Reuse id for function if possible.
            if funcname_to_id.has_key(funcname):
                id = funcname_to_id[funcname]
            else:
                id = next_id
                next_id += 1
                label = edgelist_pb2.Edgelist.Label()
                label.label = funcname
                G.graph["id_to_label"][id] = label
                funcname_to_id[funcname] = id
            G.add_edge(u, G.node[u]["return_node"], label=[id], push=None, location=d["location"])
            d["label"] = None
            if interprocedural:
                d["push"] = G.node[u]["return_node"]

    return G

def flat_walk(G, output_file=None):
    nodes_dict = defaultdict(list)
    fn_names = [x.split('.') for x in G.nodes() if len(x.split('.')) == 2]
    for n in fn_names:
        fn = n[0]
        counter = n[1]
        if "bbe" not in counter and "bbx" not in counter:
            nodes_dict[n[0]].append(n[1])

    # For each function
    for i in xrange(100):
        for fn in nodes_dict:
            wrote = False
            # Can produce multiple sentences because we duplicate for indirect calls
            counter_ids = sorted(nodes_dict[fn])
            # For each instruction in the function, sorted by bitcode order
            for counter in counter_ids:
                u_node = fn + "." + counter
                neighbors = G.adj[u_node].keys()
                # For each edge (labels are on edges). We always print the labels associated with a node.
                for v_node in neighbors:
                    edges = G[u_node][v_node]
                    edge = edges[random.sample(edges, 1)[0]]
                    if edge["label"]:
                        wrote = True
                        output_file.write("{} ".format(G.graph["id_to_label"][edge["label"][0]].label))
            if wrote:
                output_file.write("\n")
