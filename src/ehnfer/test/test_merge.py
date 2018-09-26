"""Tests for merging specifications.

Note that "specifications" are internally stores as AssociationRule objects.
"""

import pytest
import ehnfer.commands
from ehnfer.association_rule import AssociationRule

class MockModel(object):
    """Poor man's mock of the model. We only need the similarity function.

    B and D are synonyms.
    C and E are synonyms.
    """
    def __init__(self):
        self.sims = dict()
        self.sims[('B', 'D')] = 0.9
        self.sims[('C', 'E')] = 0.9
        self.sims[('D', 'B')] = 0.9
        self.sims[('E', 'C')] = 0.9

        self.sims[('A', 'A')] = 1.0
        self.sims[('B', 'C')] = 0.4
        self.sims[('C', 'B')] = 0.4

    def similarity(self, i, j):
        """Mock similarity lookup using dictionary instead of a real model."""
        return self.sims[(i, j)]


@pytest.fixture
def model():
    return MockModel()

@pytest.fixture
def threshold():
    return 0.8

def test_synonym_mapping(model, threshold):
    """Test that synonym mapping is working. Could use some more tests for this function."""

    set1 = set(['A', 'B', 'C'])
    set2 = set(['A', 'D', 'E'])

    mapping = ehnfer.commands.synonym_mapping(set1, set2, model, threshold)

    # See function doc for why this is suboptimal
    # Same behavior as before
    assert frozenset(['A', 'B']) in mapping
    assert frozenset(['A', 'D']) in mapping
    assert frozenset(['A', 'C']) in mapping
    assert frozenset(['A', 'B']) in mapping

def test_merge_not_synonyms(model, threshold):
    """Test that rules without synonyms are not merged."""
    rule1 = AssociationRule(set(['A']), set(['B']))
    rule2 = AssociationRule(set(['A']), set(['C']))

    merged_rules = ehnfer.commands.merge([rule1, rule2], model, threshold)
    assert len(merged_rules) == 0

def test_merge_synonyms(model, threshold):
    """Test that rules with synonyms are merged."""
    rule1 = AssociationRule(set(['A']), set(['B']))
    rule2 = AssociationRule(set(['A']), set(['D']))

    merged_rules = ehnfer.commands.merge([rule1, rule2], model, threshold)

    assert len(merged_rules) == 1
    assert rule1 in merged_rules[0]
    assert rule2 in merged_rules[0]

def test_merge_both_sides_pair_spec(model, threshold):
    """Test that pair rules (one context element and one response element) merge on both sides."""
    rule1 = AssociationRule(set(['B']), set(['C']))
    rule2 = AssociationRule(set(['D']), set(['E']))

    merged_rules = ehnfer.commands.merge([rule1, rule2], model, threshold)

    assert len(merged_rules) == 1
    assert rule1 in merged_rules[0]
    assert rule2 in merged_rules[0]
    assert AssociationRule(set(['B']), set(['E'])) in merged_rules[0]
    assert AssociationRule(set(['D']), set(['C'])) in merged_rules[0]
