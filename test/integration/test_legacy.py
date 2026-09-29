"""The original integration tests (formerly integration-test/bam-readcount_test.py,
Python 2), ported unchanged in intent, plus test-data/cram_site_test.sh.

They use the small NA12878/NA12892 BAM in test-data/ (see
https://github.com/genome/somatic-snv-test-data) and the two-library CRAM.
"""

import pytest

from conftest import TEST_DATA

REGIONS = ["21:10402985-10402985", "21:10405200-10405200"]

CASES = [
    # name, args, expected file
    ("normal", ["-w", "1", "-f", "ref.fa", "-l", "site_list", "test.bam"], "expected_all_lib"),
    ("perlib", ["-w", "1", "-p", "-f", "ref.fa", "-l", "site_list", "test.bam"], "expected_per_lib"),
    ("normal_as_list", ["-w", "1", "-f", "ref.fa", "test.bam"] + REGIONS, "expected_all_lib"),
    ("when_lib_absent", ["-w", "1", "-f", "ref.fa", "test_bad_rg.bam"] + REGIONS, "expected_all_lib"),
    ("indel_centric_normal", ["-w", "1", "-i", "-f", "ref.fa", "-l", "site_list", "test.bam"],
     "expected_insertion_centric_all_lib"),
    ("indel_centric_per_lib", ["-w", "1", "-i", "-p", "-f", "ref.fa", "-l", "site_list", "test.bam"],
     "expected_insertion_centric_per_lib"),
    # from cram_site_test.sh; expected outputs generated from master
    ("twolib_cram", ["-l", "twolib_site_list.txt", "-f", "rand1k.fa", "twolib.sorted.cram"],
     "expected_twolib_cram"),
    ("twolib_cram_per_lib", ["-p", "-l", "twolib_site_list.txt", "-f", "rand1k.fa", "twolib.sorted.cram"],
     "expected_twolib_cram_per_lib"),
]


@pytest.mark.parametrize("args,expected", [c[1:] for c in CASES], ids=[c[0] for c in CASES])
def test_legacy(brc, args, expected):
    proc = brc.run(args, cwd=TEST_DATA)
    assert proc.stdout == (TEST_DATA / expected).read_text()
