import ehnfer.commands

def get_rank_for_rule(rules, rule):
    """Assumes that specs is sorted by support (true in this script).
    Not efficient.

    :param specs: A list of specs sorted by support (grouped by merges)
    :param rule: An association rule to find the rank of
    """
    rank = 0
    for r in rules:
        rank += 1
        if rule.context == r.context and rule.response == r.response:
            return rank

    return None
# ----
fs_unmerged_specs = ehnfer.commands.mine(
    db_file="data/5fs.sqlite",
    support_threshold=20,
    similarity_threshold=2.0,
    model_file="data/pathbased.model",
    num_epochs=10,
    max_items=2
)

fs_unmerged_specs_seen = set()
fs_unmerged_specs_unique = []
for s in fs_unmerged_specs:
    for r in s.rules:
        cr = (frozenset(r.context), frozenset(r.response))
        if cr not in fs_unmerged_specs_seen:
            fs_unmerged_specs_seen.add(cr)
            fs_unmerged_specs_unique.append(r)

unmerged_output = open('output/5fs-unmerged.specs', 'w')
for r in fs_unmerged_specs_unique:
    unmerged_output.write("{}\n".format(r))
unmerged_output.close()

fs_merged_specs = ehnfer.commands.mine(
    db_file="data/5fs.sqlite",
    support_threshold=20,
    similarity_threshold=0.6,
    model_file="data/pathbased.model",
    num_epochs=10,
    max_items=2
)

fs_merged_specs_seen = set()
fs_merged_specs_unique = []
for s in fs_merged_specs:
    for r in s.rules:
        cr = (frozenset(r.context), frozenset(r.response))
        if cr not in fs_merged_specs_seen:
            fs_merged_specs_seen.add(cr)
            fs_merged_specs_unique.append(r)

merged_output = open('output/5fs-merged.specs', 'w')
for r in fs_merged_specs_unique:
    merged_output.write("{}\n".format(r))
merged_output.close()

##  ---- FILE SYSTEMS ----

tbl3_btrfs_free_rule = ehnfer.commands.AssociationRule(context=set(['btrfs_alloc_path']), response=set(['btrfs_free_path']), support=-1)
btrfs_free_unmerged_rank = get_rank_for_rule(fs_unmerged_specs_unique, tbl3_btrfs_free_rule)
btrfs_free_merged_rank = get_rank_for_rule(fs_merged_specs_unique, tbl3_btrfs_free_rule)

tbl3_btrfs_release_rule = ehnfer.commands.AssociationRule(context=set(['btrfs_alloc_path']), response=set(['btrfs_release_path']), support=-1)
btrfs_release_unmerged_rank = get_rank_for_rule(fs_unmerged_specs_unique, tbl3_btrfs_release_rule)
btrfs_release_merged_rank = get_rank_for_rule(fs_merged_specs_unique, tbl3_btrfs_release_rule)

tbl3_gfs2_uninit_rule = ehnfer.commands.AssociationRule(context=set(['gfs2_holder_init']), response=set(['gfs2_holder_uninit']), support=-1)
gfs2_uninit_unmerged_rank = get_rank_for_rule(fs_unmerged_specs_unique, tbl3_gfs2_uninit_rule)
gfs2_uninit_merged_rank = get_rank_for_rule(fs_merged_specs_unique, tbl3_gfs2_uninit_rule)

tbl3_gfs2_uninit_dq_rule = ehnfer.commands.AssociationRule(context=set(['gfs2_holder_init']), response=set(['gfs2_glock_dq_uninit']), support=-1)
gfs2_uninit_dq_unmerged_rank = get_rank_for_rule(fs_unmerged_specs_unique, tbl3_gfs2_uninit_dq_rule)
gfs2_uninit_dq_merged_rank = get_rank_for_rule(fs_merged_specs_unique, tbl3_gfs2_uninit_dq_rule)

f = open('output/5fs-table3.txt', 'w')
f.write("btrfs_alloc_path -> btrfs_free_path {} {}\n".format(btrfs_free_unmerged_rank, btrfs_free_merged_rank))
f.write("btrfs_alloc_path -> btrfs_release_path {} {}\n".format(btrfs_release_unmerged_rank, btrfs_release_merged_rank))
f.write("gfs2_holder_init -> gfs2_holder_uninit {} {}\n".format(gfs2_uninit_unmerged_rank, gfs2_uninit_merged_rank))
f.write("gfs2_holder_init -> gfs2_uninit_dq {} {}\n".format(gfs2_uninit_dq_unmerged_rank, gfs2_uninit_dq_merged_rank))
f.close()


merged_specs = ehnfer.commands.mine(
    db_file="data/pcisound.sqlite",
    support_threshold=3,
    similarity_threshold=0.6,
    model_file="data/pathbased.model",
    num_epochs=10
)

merged_specs_seen = set()
merged_specs_unique = []
for s in merged_specs:
    for r in s.rules:
        cr = (frozenset(r.context), frozenset(r.response))
        if cr not in merged_specs_seen:
            merged_specs_seen.add(cr)
            merged_specs_unique.append(r)

merged_output = open('output/merged.specs', 'w')
for r in merged_specs_unique:
    merged_output.write("{}\n".format(r))
merged_output.close()

unmerged_specs = ehnfer.commands.mine(
    db_file="data/pcisound.sqlite",
    support_threshold=3,
    similarity_threshold=2.0,
    model_file="data/pathbased.model",
    num_epochs=10
)

unmerged_specs_seen = set()
unmerged_specs_unique = []
for s in unmerged_specs:
    for r in s.rules:
        cr = (frozenset(r.context), frozenset(r.response))
        if cr not in unmerged_specs_seen:
            unmerged_specs_seen.add(cr)
            unmerged_specs_unique.append(r)

unmerged_output = open('output/unmerged.specs', 'w')
for r in unmerged_specs_unique:
    unmerged_output.write("{}\n".format(r))
unmerged_output.close()

tbl3_intel8x0_rule = ehnfer.commands.AssociationRule(context=set(['pci_request_regions', 'pci_enable_device']), response=set(['snd_intel8x0_free']), support=-1)
intel8x0_unmerged_rank = get_rank_for_rule(unmerged_specs_unique, tbl3_intel8x0_rule)
intel8x0_merged_rank = get_rank_for_rule(merged_specs_unique, tbl3_intel8x0_rule)

tbl3_intel8x0m_rule = ehnfer.commands.AssociationRule(context=set(['pci_request_regions', 'pci_enable_device']), response=set(['snd_intel8x0m_free']), support=-1)
intel8x0m_unmerged_rank = get_rank_for_rule(unmerged_specs_unique, tbl3_intel8x0m_rule)
intel8x0m_merged_rank = get_rank_for_rule(merged_specs_unique, tbl3_intel8x0m_rule)

tbl3_cmipci_rule = ehnfer.commands.AssociationRule(context=set(['pci_request_regions', 'pci_enable_device']), response=set(['snd_cmipci_free']), support=-1)
cmipci_unmerged_rank = get_rank_for_rule(unmerged_specs_unique, tbl3_cmipci_rule)
cmipci_merged_rank = get_rank_for_rule(merged_specs_unique, tbl3_cmipci_rule)

tbl3_ice1712_rule = ehnfer.commands.AssociationRule(context=set(['pci_request_regions', 'pci_enable_device']), response=set(['snd_ice1712_free']), support=-1)
ice1712_unmerged_rank = get_rank_for_rule(unmerged_specs_unique, tbl3_ice1712_rule)
ice1712_merged_rank = get_rank_for_rule(merged_specs_unique, tbl3_ice1712_rule)

tbl3_via82xx_rule = ehnfer.commands.AssociationRule(context=set(['pci_request_regions', 'pci_enable_device']), response=set(['snd_via82xx_free']), support=-1)
via82xx_unmerged_rank = get_rank_for_rule(unmerged_specs_unique, tbl3_via82xx_rule)
via82xx_merged_rank = get_rank_for_rule(merged_specs_unique, tbl3_via82xx_rule)


f = open('output/table3-drivers.txt', 'w')
f.write("intel8x0 {} {}\n".format(intel8x0_unmerged_rank, intel8x0_merged_rank))
f.write("intel8x0m {} {}\n".format(intel8x0m_unmerged_rank, intel8x0m_merged_rank))
f.write("cmipci {} {}\n".format(cmipci_unmerged_rank, cmipci_merged_rank))
f.write("ice1712 {} {}\n".format(ice1712_unmerged_rank, ice1712_merged_rank))
f.write("via82xx {} {}\n".format(via82xx_unmerged_rank, via82xx_merged_rank))
f.close()


