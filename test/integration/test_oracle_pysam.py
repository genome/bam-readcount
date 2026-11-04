"""Independent oracle: recompute counts with pysam and compare.

bam-readcount piles up reads with htslib's bam_plbuf (no BAQ, no overlap
detection, no orphan filtering) and then drops UNMAP/SECONDARY/QCFAIL/DUP
reads, reads below -q, bases below -b, and deletions. We reproduce that with
pysam's "nofilter" stepper and apply the same filters by hand.
"""

import random

import pysam
import pytest

from conftest import BAM, HCC, REF

CANON = {"A": "A", "C": "C", "G": "G", "T": "T"}
SKIP_FLAGS = 0x4 | 0x100 | 0x200 | 0x400


def oracle_site(bam, fa, chrom, pos, min_mapq=0, min_bq=0):
    """Return (depth, {allele: [count, plus, minus, sum_mapq, sum_bq]}) at 1-based pos.

    Deletions are reported at the first deleted base, i.e. they are found in
    the pileup of the preceding position."""
    stats = {}
    passing = 0
    deletions_here = 0

    def add(allele, r, bq):
        s = stats.setdefault(allele, [0, 0, 0, 0, 0])
        s[0] += 1
        s[2 if r.is_reverse else 1] += 1
        s[3] += r.mapping_quality
        s[4] += bq

    for col in bam.pileup(chrom, pos - 2, pos, truncate=True, stepper="nofilter",
                          max_depth=10 ** 8, ignore_overlaps=False, ignore_orphans=False,
                          min_base_quality=0, min_mapping_quality=0):
        here = col.reference_pos == pos - 1
        for pr in col.pileups:
            r = pr.alignment
            if pr.is_del or pr.is_refskip or r.flag & SKIP_FLAGS:
                continue
            if r.mapping_quality < min_mapq:
                continue
            bq = r.query_qualities[pr.query_position]
            if bq < min_bq:
                continue
            if here:
                passing += 1
                add(CANON.get(r.query_sequence[pr.query_position], "N"), r, bq)
                if pr.indel > 0:
                    q = pr.query_position
                    seq = r.query_sequence[q + 1:q + 1 + pr.indel]
                    add("+" + "".join(CANON.get(b, "N") for b in seq), r, 0)
            elif pr.indel < 0:
                p0 = col.reference_pos
                allele = "-" + fa.fetch(chrom, p0 + 1, p0 + 1 - pr.indel)
                add(allele, r, 0)
                deletions_here += 1
    return passing + deletions_here, stats


def sample_sites():
    rng = random.Random(113)
    lines = (HCC / "sites_large.txt").read_text().splitlines()
    sites = [l.split("\t") for l in rng.sample(lines, 200)]
    for name in ["sites_snv.txt", "sites_indel.txt"]:
        sites += [l.split("\t") for l in (HCC / name).read_text().splitlines()]
    return [(c, int(s)) for c, s, _ in sites]


SITES = sample_sites()


@pytest.fixture(scope="module")
def handles():
    return (pysam.AlignmentFile(str(HCC / BAM)), pysam.FastaFile(str(HCC / REF)))


@pytest.mark.parametrize("min_mapq,min_bq", [(0, 0), (20, 0), (0, 20), (20, 20), (1, 13)])
def test_counts_match_pysam(brc, tmp, handles, min_mapq, min_bq):
    bam, fa = handles
    site_file = tmp / "sites.txt"
    site_file.write_text("".join("%s\t%d\t%d\n" % (c, p, p) for c, p in SITES))
    records = brc.records(["-w", "0", "-q", str(min_mapq), "-b", str(min_bq),
                           "-f", str(HCC / REF), "-l", str(site_file), str(HCC / BAM)])
    got = {r.key: r for r in records}
    checked = 0
    for chrom, pos in SITES:
        depth, stats = oracle_site(bam, fa, chrom, pos, min_mapq, min_bq)
        if depth == 0:
            assert (chrom, pos) not in got or got[(chrom, pos)].depth == 0
            continue
        rec = got[(chrom, pos)]
        assert rec.depth == depth, (chrom, pos)
        alleles = rec.alleles()
        for allele, (n, plus, minus, smq, sbq) in stats.items():
            a = alleles[allele]
            assert (a.count, a.plus, a.minus) == (n, plus, minus), (chrom, pos, allele)
            assert abs(a.avg_mapq - smq / n) <= 0.0051, (chrom, pos, allele)
            if not a.is_indel:
                assert abs(a.avg_bq - sbq / n) <= 0.0051, (chrom, pos, allele)
        for allele, a in alleles.items():
            if allele not in stats:
                assert a.count == 0, (chrom, pos, allele)
        checked += 1
    assert checked > 150
