"""Internal-consistency properties of bam-readcount output.

These hold regardless of the golden files, so they catch errors that were
already present when the goldens were generated.
"""

import pysam
import pytest

from brc_parse import BASES
from conftest import BAM, CRAM, HCC, NOLB, REF, TWOLIB


def run(brc, *args, sites="dense", bam=BAM):
    return brc.records(["-w", "0"] + list(args) + ["-f", REF, "-l", "sites_%s.txt" % sites, bam])


def read_lines(name):
    return (HCC / name).read_text().splitlines(True)


def read_sites(name):
    out = []
    for line in (HCC / ("sites_%s.txt" % name)).read_text().splitlines():
        f = line.split()
        out.append((f[0], int(f[1]), int(f[2])))
    return out


SITE_SETS = ["snv", "indel", "dense", "large"]


@pytest.fixture(scope="module")
def default_out(brc):
    return {s: run(brc, sites=s) for s in SITE_SETS}


@pytest.mark.parametrize("sites", SITE_SETS)
def test_line_structure(default_out, sites):
    for rec in default_out[sites]:
        alleles = rec.libs["all"]
        assert [a.allele for a in alleles[:6]] == BASES, rec.raw[:200]
        for a in alleles[6:]:
            assert a.is_indel and len(a.allele) > 1, rec.raw[:200]
            assert set(a.allele[1:]) <= set("ACGTN"), a.allele
        assert rec.ref in "ACGTN"


@pytest.mark.parametrize("sites", SITE_SETS)
def test_reference_base_matches_fasta(default_out, sites):
    fa = pysam.FastaFile(str(HCC / REF))
    for rec in default_out[sites]:
        assert rec.ref == fa.fetch(rec.chrom, rec.pos - 1, rec.pos).upper()


@pytest.mark.parametrize("sites", SITE_SETS)
def test_only_requested_positions_in_list_order(default_out, sites):
    expected = [(c, p) for c, s, e in read_sites(sites) for p in range(s, e + 1)]
    got = [r.key for r in default_out[sites]]
    # every output position was requested, and output follows the list order
    it = iter(expected)
    for key in got:
        assert any(key == k for k in it), "unexpected or out-of-order position %s" % (key,)


@pytest.mark.parametrize("sites", SITE_SETS)
def test_strand_counts_and_value_ranges(default_out, sites):
    for rec in default_out[sites]:
        for a in rec.libs["all"]:
            assert a.count == a.plus + a.minus, rec.raw[:200]
            assert 0 <= a.num_q2_reads <= a.count
            if a.count == 0:
                assert (a.avg_mapq, a.avg_bq, a.avg_se_mapq, a.avg_pos_as_fraction,
                        a.avg_num_mismatches_as_fraction, a.avg_sum_mismatch_qualities,
                        a.avg_distance_to_q2_start, a.avg_clipped_length,
                        a.avg_distance_to_3p_end) == (0,) * 9
                continue
            assert 0 <= a.avg_mapq <= 60
            assert 0 <= a.avg_bq <= 93
            if a.is_indel:
                assert a.avg_bq == 0
            assert 0 <= a.avg_se_mapq <= 60
            assert 0 <= a.avg_pos_as_fraction <= 1
            assert 0 <= a.avg_num_mismatches_as_fraction <= 1
            assert a.avg_sum_mismatch_qualities >= 0
            assert 0 <= a.avg_distance_to_q2_start <= 1
            assert 0 < a.avg_clipped_length <= 100  # reads are 100 bp
            assert 0 <= a.avg_distance_to_3p_end <= 1


def _depth_check(records, insertion_centric):
    for rec in records:
        base_sum = sum(a.count for stats in rec.libs.values() for a in stats if not a.is_indel)
        ins = sum(a.count for stats in rec.libs.values() for a in stats if a.allele[0] == "+")
        dels = sum(a.count for stats in rec.libs.values() for a in stats if a.allele[0] == "-")
        # depth = reads passing filters + deletions reported at this position;
        # with -i, reads carrying an insertion are counted only as the insertion
        expected = base_sum + dels + (ins if insertion_centric else 0)
        assert rec.depth == expected, rec.raw[:300]


@pytest.mark.parametrize("sites", ["snv", "indel", "dense"])
@pytest.mark.parametrize("flags", [[], ["-i"], ["-p"], ["-p", "-i"], ["-q", "20", "-b", "20"]],
                         ids=["default", "i", "p", "pi", "q20b20"])
def test_depth_accounting(brc, sites, flags):
    _depth_check(run(brc, *flags, sites=sites, bam=TWOLIB), "-i" in flags)


@pytest.mark.parametrize("opt,levels", [("-q", [0, 1, 20, 40, 60]), ("-b", [0, 10, 13, 20, 30, 40])])
@pytest.mark.parametrize("sites", ["dense", "indel"])
def test_filters_are_monotonic(brc, opt, levels, sites):
    prev = None
    for level in levels:
        cur = {r.key: r for r in run(brc, opt, str(level), sites=sites)}
        if prev is not None:
            for key, rec in cur.items():
                assert key in prev, "raising %s added position %s" % (opt, key)
                before = prev[key].alleles()
                assert rec.depth <= prev[key].depth
                for allele, a in rec.alleles().items():
                    assert a.count <= before[allele].count if allele in before else a.count == 0
        prev = cur


@pytest.mark.parametrize("bq", [13, 20, 30])
def test_min_base_quality_respected(brc, bq):
    for rec in run(brc, "-b", str(bq), sites="dense"):
        for a in rec.libs["all"]:
            if a.count and not a.is_indel:
                assert a.avg_bq >= bq, rec.raw[:200]


@pytest.mark.parametrize("mq", [20, 60])
def test_min_mapping_quality_respected(brc, mq):
    for rec in run(brc, "-q", str(mq), sites="dense"):
        for a in rec.libs["all"]:
            if a.count:
                assert a.avg_mapq >= mq, rec.raw[:200]


@pytest.mark.parametrize("sites", ["snv", "indel", "dense"])
@pytest.mark.parametrize("insertion_centric", [False, True])
def test_per_library_sums_to_all(brc, sites, insertion_centric):
    extra = ["-i"] if insertion_centric else []
    per_lib = run(brc, "-p", *extra, sites=sites, bam=TWOLIB)
    merged = {r.key: r for r in run(brc, *extra, sites=sites, bam=TWOLIB)}
    assert [r.key for r in per_lib] == list(merged)
    for rec in per_lib:
        assert set(rec.libs) <= {"libA", "libB"}
        assert rec.depth == merged[rec.key].depth
        total = merged[rec.key].summed()
        for allele, counts in rec.summed().items():
            if counts[0]:
                assert total[allele] == counts, (rec.key, allele)
        for allele, counts in total.items():
            if counts[0]:
                assert rec.summed().get(allele) == counts, (rec.key, allele)


@pytest.mark.parametrize("sites", ["snv", "indel", "dense"])
def test_relabelled_bams_match_original(brc, sites):
    """Changing only read groups must not change the all-library output."""
    base = [r.raw for r in run(brc, sites=sites)]
    assert [r.raw for r in run(brc, sites=sites, bam=TWOLIB)] == base
    assert [r.raw for r in run(brc, sites=sites, bam=NOLB)] == base


@pytest.mark.parametrize("sites", ["indel", "dense"])
def test_insertion_centric(brc, sites):
    default = {r.key: r for r in run(brc, sites=sites)}
    ic = run(brc, "-i", sites=sites)
    assert [r.key for r in ic] == list(default)
    for rec in ic:
        d = default[rec.key].alleles()
        for allele, a in rec.alleles().items():
            if a.is_indel:
                assert (a.count, a.plus, a.minus) == (d[allele].count, d[allele].plus, d[allele].minus)
            else:
                assert a.count <= d[allele].count
        ins = sum(a.count for a in rec.libs["all"] if a.allele[0] == "+")
        base_default = sum(a.count for a in default[rec.key].libs["all"] if not a.is_indel)
        base_ic = sum(a.count for a in rec.libs["all"] if not a.is_indel)
        assert base_ic == base_default - ins


@pytest.mark.parametrize("sites", ["snv", "indel", "dense", "large"])
@pytest.mark.parametrize("flags", [[], ["-i"], ["-q", "20", "-b", "20"]], ids=["default", "i", "q20b20"])
def test_bam_and_cram_identical(brc, tmp, sites, flags):
    site_file = HCC / ("sites_%s.txt" % sites)
    if sites == "large":
        # every CRAM site fetch decodes a whole container, so subsample
        site_file = tmp / "sites.txt"
        site_file.write_text("".join(read_lines("sites_large.txt")[::10]))
    args = ["-w", "0"] + flags + ["-f", REF, "-l", str(site_file)]
    assert brc.run(args + [CRAM]).stdout == brc.run(args + [BAM]).stdout


def test_max_depth_only_reduces_counts(brc):
    region = "chr17:62231473-62231573"  # ~480x exon
    prev = None
    for d in [10000000, 200, 50, 10, 1]:
        cur = {r.key: r for r in brc.records(["-w", "0", "-d", str(d), "-f", REF, BAM, region])}
        assert len(cur) == 101
        if prev is not None:
            for key, rec in cur.items():
                assert rec.depth <= prev[key].depth
        prev = cur
    # with the default limit the exon is deep; a small limit must reduce it
    assert max(r.depth for r in prev.values()) < 200


def test_filtered_flags_are_ignored(brc):
    """SECONDARY, QCFAIL, DUP and UNMAP copies of reads must not change anything."""
    args = ["-w", "0", "-f", REF, "-l", "sites_flags.txt"]
    base = brc.run(args + ["base.bam"]).stdout
    assert base.count("\n") > 100
    assert brc.run(args + ["flags.bam"]).stdout == base


def test_supplementary_reads_are_counted(brc):
    """Characterisation: bam-readcount does not filter SUPPLEMENTARY alignments,
    so duplicating every read as a supplementary copy doubles every count."""
    args = ["-w", "0", "-f", REF, "-l", "sites_flags.txt"]
    base = brc.records(args + ["base.bam"])
    supp = brc.records(args + ["supplementary.bam"])
    assert [r.key for r in supp] == [r.key for r in base]
    for b, s in zip(base, supp):
        assert s.depth == 2 * b.depth
        for allele, a in b.alleles().items():
            assert s.alleles()[allele].count == 2 * a.count
