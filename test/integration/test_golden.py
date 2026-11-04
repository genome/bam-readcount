"""Golden regression: output must match the stored expected output byte-for-byte.

Expected outputs were generated from master (see update_golden.py) and
cross-checked against the mgibio/bam-readcount:1.0.1 release image.
"""

import difflib
import gzip

import pytest

from conftest import EXPECTED
from golden_cases import CASES


def read_expected(name):
    with gzip.open(EXPECTED / (name + ".tsv.gz"), "rt") as fh:
        return fh.read()


@pytest.mark.parametrize("name,args", CASES, ids=[c[0] for c in CASES])
def test_golden(brc, name, args):
    expected = read_expected(name)
    actual = brc.run(args).stdout
    if actual != expected:
        diff = "".join(difflib.unified_diff(
            expected.splitlines(True), actual.splitlines(True),
            "expected/%s" % name, "actual", n=0))
        pytest.fail("output differs from golden %s:\n%s" % (name, diff[:8000]))


def test_every_golden_file_has_a_case():
    names = {c[0] for c in CASES}
    files = {p.name[:-len(".tsv.gz")] for p in EXPECTED.glob("*.tsv.gz")}
    assert files == names
