Integration tests
=================

A pytest suite (Python 3; needs `pytest`, `pytest-xdist` and `pysam`) that
runs a built `bam-readcount` against committed fixtures. `ctest` runs it
automatically after a build. To build and run everything in a container:

    docker build -t bam-readcount-test -f docker/Dockerfile.test docker
    docker run --rm -v "$PWD":/src -w /src bam-readcount-test sh -c \
        'mkdir -p build && cd build && cmake .. && make && ctest --output-on-failure'

    # or just the pytest suite against any executable
    python3 -m pytest -n auto test/integration --brc path/to/bam-readcount

| Module | What it checks |
|---|---|
| `test_legacy.py` | the original Python 2 tests on `test-data/test.bam`, plus the two-library CRAM |
| `test_golden.py` | byte-for-byte output for a matrix of inputs x options (`golden_cases.py`) |
| `test_invariants.py` | internal consistency: depth accounting, strand sums, value ranges, filter monotonicity, per-library sums, BAM == CRAM, flag filtering |
| `test_oracle_pysam.py` | base, indel and strand counts, depth and mean MAPQ/BQ recomputed independently with pysam |
| `test_site_lists.py` | `-l` behaviour: batch vs one-site runs, ordering, duplicates, overlaps, ranges, chunking, positional regions |
| `test_cli_errors.py` | help/version, exit codes, missing files/index, malformed site lists, warning limits, missing tags |

Tests marked `xfail(strict=True)` document known bugs; they will start
failing (XPASS) when the bug is fixed, as a prompt to remove the marker.

Fixtures
--------

`test-data/hcc1395` holds small slices of the HCC1395 normal exome
(chr6 + chr17) from genomedata.org. `windows.bed` lists each slice and why it
was chosen. The reference is the full-length chr6/chr17 sequence with
everything outside the slices replaced by N and bgzip-compressed, so
coordinates match the original data.

    test/integration/fixtures/make_hcc1395_fixtures.sh   # regenerate fixtures (~700 MB download)
    python3 test/integration/update_golden.py --brc build/bin/bam-readcount   # regenerate goldens

Only regenerate goldens from a build whose output you trust, and review the
diff. The committed goldens were generated from master and are identical to
the output of the `mgibio/bam-readcount:1.0.1` release.
