"""Shared pytest configuration for the bam-readcount integration suite.

The binary under test is chosen, in order of precedence, by
  --brc PATH            path to a bam-readcount executable
  --brc-image IMAGE     a Docker image whose entrypoint is bam-readcount
  $BAM_READCOUNT        path to a bam-readcount executable
  build/bin/bam-readcount under the repository root
"""

import os
import subprocess
from pathlib import Path

import pytest

import brc_parse

REPO = Path(__file__).resolve().parents[2]
TEST_DATA = REPO / "test-data"
HCC = TEST_DATA / "hcc1395"
EXPECTED = HCC / "expected"


def pytest_addoption(parser):
    parser.addoption("--brc", default=None, help="path to the bam-readcount executable")
    parser.addoption("--brc-image", default=None,
                     help="Docker image to run bam-readcount from (entrypoint must be bam-readcount)")


class Runner:
    def __init__(self, exe=None, image=None):
        self.exe = exe
        self.image = image

    def command(self, args, cwd):
        args = [str(a) for a in args]
        if self.image:
            return ["docker", "run", "--rm", "-v", "%s:%s" % (REPO, REPO),
                    "-w", str(cwd), self.image] + args
        return [self.exe] + args

    def run(self, args, cwd=None, check=True):
        """Run bam-readcount; returns a CompletedProcess with text stdout/stderr."""
        cwd = Path(cwd or HCC)
        proc = subprocess.run(self.command(args, cwd), cwd=str(cwd),
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              universal_newlines=True, timeout=600)
        if check and proc.returncode != 0:
            raise AssertionError("bam-readcount %s exited %d\nstderr:\n%s"
                                 % (" ".join(map(str, args)), proc.returncode, proc.stderr[-4000:]))
        return proc

    def records(self, args, cwd=None):
        per_lib = "-p" in [str(a) for a in args]
        return brc_parse.parse(self.run(args, cwd).stdout, per_lib=per_lib)


@pytest.fixture(scope="session")
def brc(request):
    image = request.config.getoption("--brc-image")
    exe = request.config.getoption("--brc") or os.environ.get("BAM_READCOUNT")
    if image:
        return Runner(image=image)
    if not exe:
        exe = str(REPO / "build" / "bin" / "bam-readcount")
    exe = str(Path(exe).resolve())
    if not os.access(exe, os.X_OK):
        pytest.exit("bam-readcount executable not found: %s (use --brc)" % exe, returncode=2)
    return Runner(exe=exe)


@pytest.fixture
def tmp(tmp_path):
    return tmp_path


# Fixture file names, relative to test-data/hcc1395
REF = "ref.masked.fa.gz"
BAM = "exome_norm.slice.bam"
CRAM = "exome_norm.slice.cram"
TWOLIB = "twolib.slice.bam"
NOLB = "nolb.slice.bam"
FLAGS = "flags.bam"
