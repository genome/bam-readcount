"""Site-list (-l) processing.

These target the -l code path, where each site is fetched separately from
the index. PR #113 enables an htslib block cache on this path, so results
must not depend on what was read before: order, repetition, batching and
chunking of the list must not change the output for any site.
"""

import random

import pytest

from conftest import BAM, HCC, REF


def lines_of(name):
    return [l for l in (HCC / name).read_text().splitlines() if l.strip()]


def run_list(brc, tmp, lines, bam=BAM, extra=(), name="sites.txt"):
    path = tmp / name
    path.write_text("".join(l + "\n" for l in lines))
    return brc.run(["-w", "0"] + list(extra) + ["-f", REF, "-l", str(path), bam]).stdout.splitlines()


def by_key(out_lines):
    d = {}
    for l in out_lines:
        f = l.split("\t", 2)
        d.setdefault((f[0], int(f[1])), l)
    return d


@pytest.fixture(scope="module")
def dense_reference(brc):
    out = brc.run(["-w", "0", "-f", REF, "-l", "sites_dense.txt", BAM]).stdout.splitlines()
    assert len(out) == 1000
    return by_key(out)


@pytest.mark.parametrize("extra", [[], ["-i"]], ids=["default", "i"])
def test_batch_equals_isolated_runs(brc, tmp, extra, bam=BAM):
    # BAM only: CRAM output is compared with BAM output in test_invariants.py,
    # and each CRAM run is several times slower.
    rng = random.Random(113)
    sample = rng.sample(lines_of("sites_large.txt"), 40) + lines_of("sites_indel.txt")
    batch = run_list(brc, tmp, sample, bam, extra, "batch.txt")
    isolated = []
    for i, line in enumerate(sample):
        isolated += run_list(brc, tmp, [line], bam, extra, "one.txt")
    assert batch == isolated


@pytest.mark.parametrize("name", ["sites_shuffled.txt", "sites_reversed.txt"])
def test_order_does_not_change_per_site_output(brc, tmp, dense_reference, name):
    sites = lines_of(name)
    out = run_list(brc, tmp, sites)
    expected = [dense_reference[(l.split()[0], int(l.split()[1]))] for l in sites]
    assert out == expected


def test_interleaved_chromosomes(brc, tmp, dense_reference):
    sites = lines_of("sites_interleaved.txt")
    assert len({l.split()[0] for l in sites}) == 2
    out = run_list(brc, tmp, sites)
    reference = dict(dense_reference)
    reference.update(by_key(run_list(brc, tmp, lines_of("sites_snv.txt"), name="snv.txt")))
    expected = [reference[(l.split()[0], int(l.split()[1]))] for l in sites
                if (l.split()[0], int(l.split()[1])) in reference]
    assert out == expected
    chroms = [l.split("\t")[0] for l in out]
    assert sum(1 for a, b in zip(chroms, chroms[1:]) if a != b) >= 10


def test_duplicate_sites_repeat_identical_lines(brc, tmp, dense_reference):
    sites = lines_of("sites_dupes.txt")
    out = run_list(brc, tmp, sites)
    assert out == [dense_reference[(l.split()[0], int(l.split()[1]))] for l in sites]


def test_overlapping_ranges(brc, tmp, dense_reference):
    sites = lines_of("sites_overlap.txt")
    out = run_list(brc, tmp, sites)
    expected = []
    for l in sites:
        c, s, e = l.split()
        expected += [dense_reference[(c, p)] for p in range(int(s), int(e) + 1) if (c, p) in dense_reference]
    assert out == expected


def test_range_line_equals_single_base_lines(brc, dense_reference):
    out = brc.run(["-w", "0", "-f", REF, "-l", "sites_dense_range.txt", BAM]).stdout.splitlines()
    assert out == list(dense_reference.values())


@pytest.mark.parametrize("extra", [[], ["-p"]], ids=["default", "p"])
def test_large_list_is_stable_and_chunkable(brc, tmp, extra):
    sites = lines_of("sites_large.txt")
    assert len(sites) > 5000
    whole = run_list(brc, tmp, sites, extra=extra, name="all.txt")
    assert len(whole) > 5000
    assert run_list(brc, tmp, sites, extra=extra, name="again.txt") == whole
    chunked = []
    n = len(sites) // 10 + 1
    for i in range(0, len(sites), n):
        chunked += run_list(brc, tmp, sites[i:i + n], extra=extra, name="chunk.txt")
    assert chunked == whole


def test_large_list_reversed_matches_forward(brc, tmp):
    sites = lines_of("sites_large.txt")
    fwd = run_list(brc, tmp, sites, name="fwd.txt")
    rev = run_list(brc, tmp, sites[::-1], name="rev.txt")
    assert rev == fwd[::-1]


def test_multiple_positional_regions_match_site_list_for_snvs(brc):
    regions = ["%s:%s-%s" % tuple(l.split()) for l in lines_of("sites_snv.txt")]
    positional = brc.run(["-w", "0", "-f", REF, BAM] + regions).stdout
    assert positional == brc.run(["-w", "0", "-f", REF, "-l", "sites_snv.txt", BAM]).stdout


def test_positional_range_matches_site_list(brc, dense_reference):
    c, s, e = lines_of("sites_dense_range.txt")[0].split()
    out = brc.run(["-w", "0", "-f", REF, BAM, "%s:%s-%s" % (c, s, e)]).stdout.splitlines()
    assert out == list(dense_reference.values())


@pytest.mark.xfail(strict=True, reason=(
    "pre-existing bug: with several positional regions the deletion queue is not "
    "cleared between regions (the -l path clears it), so a deletion seen while "
    "processing one region is counted again in an adjacent one and depth is inflated"))
def test_multiple_positional_regions_match_site_list_for_indels(brc):
    regions = ["%s:%s-%s" % tuple(l.split()) for l in lines_of("sites_indel.txt")]
    positional = brc.run(["-w", "0", "-f", REF, BAM] + regions).stdout
    assert positional == brc.run(["-w", "0", "-f", REF, "-l", "sites_indel.txt", BAM]).stdout
