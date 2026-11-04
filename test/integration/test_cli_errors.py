"""Command-line handling, exit codes, warnings and malformed input."""

import pysam
import pytest

from conftest import BAM, HCC, NOLB, REF


def test_help(brc):
    for args in (["-h"], ["--help"], []):
        proc = brc.run(args, check=False)
        assert proc.returncode == 1
        assert "Usage: bam-readcount" in proc.stdout
        assert "--site-list" in proc.stdout


def test_version(brc):
    proc = brc.run(["-v"], check=False)
    assert proc.returncode == 1
    assert proc.stdout.startswith("bam-readcount version:")


def test_missing_bam(brc):
    proc = brc.run(["-f", REF, "does-not-exist.bam", "chr17:1-10"], check=False)
    assert proc.returncode == 1
    assert "Fail to open BAM file" in proc.stderr


@pytest.mark.xfail(strict=True, reason="pre-existing bug: the error names argv[optind] (e.g. '-f') "
                                       "instead of the BAM path")
def test_missing_bam_message_names_file(brc):
    proc = brc.run(["-f", REF, "does-not-exist.bam", "chr17:1-10"], check=False)
    assert "Fail to open BAM file does-not-exist.bam" in proc.stderr


@pytest.mark.parametrize("mode", ["site-list", "region"])
def test_missing_index(brc, mode):
    target = ["-l", "sites_snv.txt"] if mode == "site-list" else []
    region = [] if mode == "site-list" else ["chr17:47840089-47840089"]
    proc = brc.run(["-f", REF] + target + ["noindex.bam"] + region, check=False)
    assert proc.returncode == 1
    assert "BAM indexing file is not available" in proc.stderr
    assert proc.stdout == ""


def test_missing_site_list(brc):
    proc = brc.run(["-f", REF, "-l", "no-such-sites.txt", BAM], check=False)
    assert proc.returncode == 1
    assert "Failed to open region list file: no-such-sites.txt" in proc.stderr


def test_invalid_positional_region(brc):
    proc = brc.run(["-f", REF, BAM, "chrNope:1-10"], check=False)
    assert proc.returncode == 1
    assert "Invalid region chrNope:1-10" in proc.stderr


def test_edge_site_list(brc):
    """Comments, blank lines, too-few-fields and non-numeric lines are skipped;
    unknown contigs are reported and skipped; start > end, zero-coverage sites,
    contig ends and positions past the contig end produce no output."""
    proc = brc.run(["-w", "0", "-f", REF, "-l", "sites_edge.txt", BAM])
    assert "chrUn_notreal not found in bam file. Region chrUn_notreal 100 100 skipped." in proc.stderr
    keys = [tuple(l.split("\t")[:2]) for l in proc.stdout.splitlines()]
    edge = (HCC / "sites_edge.txt").read_text().splitlines()
    valid = [l.split()[:2] for l in edge[-2:]]  # the tab- and the space-separated valid sites
    assert keys == [tuple(v) for v in valid]


def test_empty_site_list(brc, tmp):
    empty = tmp / "empty.txt"
    empty.write_text("")
    proc = brc.run(["-f", REF, "-l", str(empty), BAM])
    assert proc.stdout == ""


def test_bgzipped_reference_is_used(brc):
    out = brc.records(["-w", "0", "-f", REF, "-l", "sites_snv.txt", BAM])
    assert out and all(r.ref in "ACGT" for r in out)


@pytest.mark.xfail(strict=True, reason="pre-existing bug: without -f the read pre-processing "
                                       "dereferences a NULL reference and the program segfaults")
def test_runs_without_reference(brc):
    proc = brc.run(["-w", "0", "-l", "sites_snv.txt", BAM], check=False)
    assert proc.returncode == 0
    assert proc.stdout.count("\n") == 30


def test_distribution_option_is_unsupported(brc):
    """-D throws "Not currently supporting distributions" and aborts."""
    proc = brc.run(["-D", "1", "-w", "0", "-f", REF, "-l", "sites_snv.txt", BAM], check=False)
    assert proc.returncode != 0


def test_min_mapping_quality_is_reported(brc):
    proc = brc.run(["-q", "17", "-f", REF, "-l", "sites_snv.txt", BAM])
    assert "Minimum mapping quality is set to 17" in proc.stderr


def test_expected_libraries_are_reported(brc):
    proc = brc.run(["-f", REF, "-l", "sites_snv.txt", "twolib.slice.bam"])
    assert "Expect library: libA in BAM" in proc.stderr
    assert "Expect library: libB in BAM" in proc.stderr


SM_MSG = "Couldn't find single-end mapping quality"


def warnings(stderr, msg):
    return sum(1 for l in stderr.splitlines() if l.startswith("WARNING") and msg in l)


@pytest.mark.parametrize("limit", [0, 1, 5, 100])
def test_warning_limit(brc, limit):
    # The source BAM has no SM tags, so every properly paired read warns.
    proc = brc.run(["-w", str(limit), "-f", REF, "-l", "sites_flags.txt", "base.bam"])
    assert warnings(proc.stderr, SM_MSG) == limit
    disabled = proc.stderr.count("will be disabled")
    assert disabled == (1 if limit > 0 else 0)
    if limit:
        assert "has been emitted %d times and will be disabled" % limit in proc.stderr


def test_unlimited_warnings(brc):
    proc = brc.run(["-f", REF, "-l", "sites_flags.txt", "base.bam"])
    assert warnings(proc.stderr, SM_MSG) > 1000
    assert "will be disabled" not in proc.stderr
    proc = brc.run(["-w", "-1", "-f", REF, "-l", "sites_flags.txt", "base.bam"])
    assert warnings(proc.stderr, SM_MSG) > 1000


def test_missing_nm_tag_warns_and_zeroes_mismatch_fraction(brc, tmp):
    src = pysam.AlignmentFile(str(HCC / "base.bam"))
    out_path = tmp / "nonm.bam"
    with pysam.AlignmentFile(str(out_path), "wb", template=src) as out:
        for r in src:
            r.set_tag("NM", None)
            out.write(r)
    pysam.index(str(out_path))
    proc = brc.run(["-w", "3", "-f", str(HCC / REF), "-l", str(HCC / "sites_flags.txt"), str(out_path)])
    assert warnings(proc.stderr, "Couldn't find number of mismatches") == 3
    base = brc.records(["-w", "0", "-f", REF, "-l", "sites_flags.txt", "base.bam"])
    got = brc.records(["-w", "0", "-f", str(HCC / REF), "-l", str(HCC / "sites_flags.txt"), str(out_path)])
    assert [r.key for r in got] == [r.key for r in base]
    for b, g in zip(base, got):
        for allele, a in g.alleles().items():
            assert a.count == b.alleles()[allele].count
            assert a.avg_num_mismatches_as_fraction == 0


def test_per_library_without_lb_tag(brc):
    """Characterisation: with -p and no LB in the header every read warns
    'Library unavailable' and no lines are written."""
    proc = brc.run(["-p", "-w", "2", "-f", REF, "-l", "sites_snv.txt", NOLB])
    assert warnings(proc.stderr, "Library unavailable") == 2
    assert proc.stdout == ""
