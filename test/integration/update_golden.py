#!/usr/bin/env python3
"""Regenerate (or write elsewhere, for comparison) the golden outputs.

    update_golden.py --brc build/bin/bam-readcount
    update_golden.py --brc-image mgibio/bam-readcount:1.0.1 --out /tmp/golden-1.0.1

Goldens are written gzip-compressed (without timestamps, so they are
byte-for-byte reproducible) to test-data/hcc1395/expected by default.
Only regenerate from a build you trust; review the diff before committing.
"""

import argparse
import gzip
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from conftest import EXPECTED, Runner  # noqa: E402
from golden_cases import CASES  # noqa: E402


def write_gz(path, text):
    with open(path, "wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0) as gz:
            gz.write(text.encode())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--brc")
    ap.add_argument("--brc-image")
    ap.add_argument("--out", type=Path, default=EXPECTED)
    ap.add_argument("--only", help="substring filter on case names")
    args = ap.parse_args()
    if not (args.brc or args.brc_image):
        ap.error("one of --brc or --brc-image is required")
    runner = Runner(exe=str(Path(args.brc).resolve()) if args.brc else None, image=args.brc_image)
    args.out.mkdir(parents=True, exist_ok=True)
    for name, cmd in CASES:
        if args.only and args.only not in name:
            continue
        proc = runner.run(cmd)
        write_gz(args.out / (name + ".tsv.gz"), proc.stdout)
        print("%-32s %6d lines" % (name, proc.stdout.count("\n")))


if __name__ == "__main__":
    main()
