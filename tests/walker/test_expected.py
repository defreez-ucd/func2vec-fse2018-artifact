import walker.commands
from walker.pushdown import PushDown
from collections import namedtuple
import subprocess

# Run from program2vec root with pytest
# pytest tests/walker

def test_equals_expected():
    TestConfig = namedtuple('TestConfig', ['cfile', 'expected', 'errcodes',
                                           'length', 'walks', 'remove_labels',
                                           'interprocedural', 'enterexit'])
    # Strip out non-call labels for tests, since the other bitcode instructions are hard to validate by hand???
    CALLS_ONLY=[]

    test_configurations = [
       #TestConfig(cfile="tests/programs/trivial.c", errcodes=None, expected="trivial.walks", length=100, walks=100, remove_labels=CALLS_ONLY, interprocedural=True),
       TestConfig(cfile="tests/programs/original.c", errcodes=None, expected="original_intraprocedural.walks", length=100, walks=100, remove_labels=CALLS_ONLY, interprocedural=False, enterexit=False)
    ]

    for config in test_configurations:
        print(config)

        bitcode_file = "%s.bc" % config.cfile
        subprocess.Popen(["clang", "-c", "-g", "-emit-llvm", config.cfile, "-o", "%s.bc" % config.cfile])
        edgelist = walker.commands._generate_edgelist(bitcode_file, "build/getgraph", config.errcodes)
        G = walker.commands._create_graph_from_edgelist(edgelist, config.remove_labels)
        PDS = PushDown(G, config.enterexit, config.interprocedural)
        walks = PDS.random_walk_all_labels(config.length, config.walks)

        # This output is suppressed if the test passes
        for w in walks:
            print(w)

        walks_from_file = walker.commands._read_walks_from_file("tests/walker/expected/%s" % config.expected)

        assert walks == walks_from_file
