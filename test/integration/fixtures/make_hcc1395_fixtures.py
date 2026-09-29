#!/usr/bin/env python3
"""Generate the committed HCC1395 test fixtures in test-data/hcc1395.

Run through make_hcc1395_fixtures.sh, which downloads the source data and
runs this script inside the bam-readcount-test Docker image so the tool
versions (samtools, pysam, bgzip) are fixed.

Inputs (in --cache):
  Exome_Norm_sorted_mrkdup_bqsr.bam/.bai   HCC1395 normal exome, chr6+chr17
  ref_genome.fa (+.fai)                    matching reference
  exome_regions.bed                        capture targets
  Exome_Norm_HC_calls.vcf                  germline calls, used to pick sites

Everything is deterministic: windows are chosen by fixed rules and a fixed
random seed, so rerunning produces the same fixtures.
"""

import argparse
import hashlib
import os
import random
import subprocess
import sys
import zlib
from pathlib import Path

import pysam

SEED = 1395
PAD = 750            # window half-width around a chosen variant
REF_PAD = 2000       # real reference kept around every read span
SIZE_BUDGET = 10 * 1024 * 1024


def sh(cmd):
    print("+", cmd, file=sys.stderr)
    subprocess.run(cmd, shell=True, check=True)


def read_bed(path):
    out = []
    with open(path) as fh:
        for line in fh:
            if not line.strip() or line.startswith(("#", "track", "browser")):
                continue
            f = line.split("\t")
            out.append((f[0], int(f[1]), int(f[2])))
    return out


def in_targets(targets_by_chrom, chrom, pos0):
    for s, e in targets_by_chrom.get(chrom, []):
        if s <= pos0 < e:
            return True
    return False


def depth_at(bam, chrom, pos0):
    n = 0
    for r in bam.fetch(chrom, pos0, pos0 + 1):
        if not (r.is_duplicate or r.is_unmapped or r.is_secondary or r.is_qcfail):
            n += 1
    return n


def classify(rec):
    if not rec.alts or len(rec.alts) != 1:
        return None
    ref = rec.ref
    alt = rec.alts[0]
    if alt.startswith("<") or "*" in alt:
        return None
    gt = rec.samples[0]["GT"]
    if None in gt:
        return None
    zyg = "hom" if gt[0] == gt[1] else "het"
    if len(ref) == 1 and len(alt) == 1:
        return "snv_" + zyg
    if len(ref) == 1 and len(alt) > 1:
        return "ins_1" if len(alt) == 2 else "ins_multi"
    if len(alt) == 1 and len(ref) > 1:
        return "del_1" if len(ref) == 2 else "del_multi"
    return None


def pick_windows(bam, vcf_path, targets):
    """Return a list of (chrom, start0, end0, reason)."""
    rng = random.Random(SEED)
    by_chrom = {}
    for c, s, e in targets:
        by_chrom.setdefault(c, []).append((s, e))

    cands = {}
    for rec in pysam.VariantFile(vcf_path):
        if "PASS" not in rec.filter.keys() and len(rec.filter.keys()) > 0:
            continue
        cls = classify(rec)
        if cls and in_targets(by_chrom, rec.chrom, rec.pos - 1):
            cands.setdefault((cls, rec.chrom), []).append(rec.pos)

    # how many windows of each class per chromosome
    want = {
        "snv_het": {"chr17": 2, "chr6": 1},
        "snv_hom": {"chr17": 1, "chr6": 1},
        "ins_1": {"chr17": 1, "chr6": 1},
        "ins_multi": {"chr17": 1},
        "del_1": {"chr17": 1, "chr6": 1},
        "del_multi": {"chr17": 1, "chr6": 1},
    }
    windows = []
    for cls, per_chrom in want.items():
        for chrom, n in per_chrom.items():
            pool = sorted(cands.get((cls, chrom), []))
            rng.shuffle(pool)
            got = 0
            for pos in pool:
                if got == n:
                    break
                d = depth_at(bam, chrom, pos - 1)
                if 30 <= d <= 400:
                    windows.append((chrom, pos - 1 - PAD, pos - 1 + PAD,
                                    "%s at %s:%d depth=%d" % (cls, chrom, pos, d)))
                    got += 1
            if got < n:
                print("WARNING: only %d/%d %s windows on %s" % (got, n, cls, chrom), file=sys.stderr)

    # Sample targets for a very deep exon and one rich in MAPQ0/dup/clipped reads.
    sample = rng.sample(targets, 400)
    best_depth = None
    best_messy = None
    for c, s, e in sample:
        if e - s < 100:
            continue
        mid = (s + e) // 2
        reads = list(bam.fetch(c, mid, mid + 1))
        if not reads:
            continue
        n = len(reads)
        messy = sum(1 for r in reads if r.mapping_quality == 0 or r.is_duplicate
                    or (r.cigartuples and any(op == 4 for op, _ in r.cigartuples)))
        if n <= 2000 and (best_depth is None or n > best_depth[0]):
            best_depth = (n, c, mid)
        if n >= 20:
            frac = messy / n
            if best_messy is None or frac > best_messy[0]:
                best_messy = (frac, c, mid)
    n, c, mid = best_depth
    windows.append((c, mid - 500, mid + 500, "high depth exon (%d reads at %s:%d)" % (n, c, mid + 1)))
    frac, c, mid = best_messy
    windows.append((c, mid - 500, mid + 500,
                    "MAPQ0/dup/soft-clip rich (%.0f%% at %s:%d)" % (frac * 100, c, mid + 1)))

    # A zero-coverage intergenic window: middle of the largest gap between chr6 targets.
    t6 = sorted(by_chrom["chr6"])
    gap = max(((t6[i + 1][0] - t6[i][1], t6[i][1], t6[i + 1][0]) for i in range(len(t6) - 1)))
    mid = (gap[1] + gap[2]) // 2
    while bam.count("chr6", mid - 500, mid + 500) > 0:
        mid += 100000
    windows.append(("chr6", mid - 500, mid + 500, "zero coverage intergenic"))

    windows.sort(key=lambda w: (w[0] != "chr6", w[1]))  # header order: chr6, chr17
    return windows


def write_slice(bam, windows, out_path):
    """All reads overlapping any window, sorted, no duplicates across windows."""
    seen = set()
    reads = []
    for c, s, e, _ in windows:
        for r in bam.fetch(c, s, e):
            k = (r.query_name, r.flag, r.reference_id, r.reference_start)
            if k not in seen:
                seen.add(k)
                reads.append(r)
    reads.sort(key=lambda r: (r.reference_id, r.reference_start, r.query_name, r.flag))
    header = bam.header.to_dict()
    header["PG"] = [{"ID": "make_hcc1395_fixtures", "PN": "make_hcc1395_fixtures.py"}]
    tmp = str(out_path) + ".unsorted.bam"
    with pysam.AlignmentFile(tmp, "wb", header=header) as out:
        for r in reads:
            out.write(r)
    sh("samtools sort -o %s %s && rm %s && samtools index %s" % (out_path, tmp, tmp, out_path))
    return reads


def write_masked_reference(fasta_path, bam_path, windows, out_path):
    """Full-length contigs; real sequence only near window reads, N elsewhere."""
    fa = pysam.FastaFile(fasta_path)
    spans = {}
    with pysam.AlignmentFile(bam_path) as bam:
        for c, s, e, _ in windows:
            lo, hi = s, e
            for r in bam.fetch(c, s, e):
                lo = min(lo, r.reference_start)
                hi = max(hi, r.reference_end or r.reference_start + 1)
            spans.setdefault(c, []).append((max(0, lo - REF_PAD), hi + REF_PAD))
    plain = str(out_path)[:-3]
    with open(plain, "w") as out:
        for c in fa.references:
            length = fa.get_reference_length(c)
            seq = bytearray(b"N" * length)
            for lo, hi in spans.get(c, []):
                hi = min(hi, length)
                seq[lo:hi] = fa.fetch(c, lo, hi).upper().encode()
            out.write(">%s\n" % c)
            for i in range(0, length, 60):
                out.write(seq[i:i + 60].decode())
                out.write("\n")
    sh("rm -f %s %s.fai %s.gzi && bgzip -@4 -i %s && samtools faidx %s"
       % (out_path, out_path, out_path, plain, out_path))
    return spans


def write_relabelled(src, out_path, rgs, assign):
    """Copy src with new @RG lines; assign(read) -> RG id."""
    with pysam.AlignmentFile(src) as bam:
        header = bam.header.to_dict()
        header["RG"] = rgs
        with pysam.AlignmentFile(out_path, "wb", header=header) as out:
            for r in bam:
                r.set_tag("RG", assign(r), "Z")
                out.write(r)
    sh("samtools index %s" % out_path)


def lib_for(name):
    return "rgA" if zlib.crc32(name.encode()) % 2 == 0 else "rgB"


def write_flag_fixtures(src, window, outdir):
    """base.bam: non-duplicate primary reads from one window.
    flags.bam: base + copies flagged SECONDARY, QCFAIL, DUP, UNMAP (must be ignored).
    supplementary.bam: base + copies flagged SUPPLEMENTARY (characterisation)."""
    c, s, e, _ = window
    mid = (s + e) // 2
    with pysam.AlignmentFile(src) as bam:
        header = bam.header.to_dict()
        base = [r for r in bam.fetch(c, mid - 150, mid + 150)
                if not (r.flag & (0x4 | 0x100 | 0x200 | 0x400 | 0x800))]
    outs = {"base.bam": [], "flags.bam": [], "supplementary.bam": []}
    for r in base:
        outs["base.bam"].append(r)
        outs["flags.bam"].append(r)
        outs["supplementary.bam"].append(r)
    for bit, tag, target in [(0x100, "sec", "flags.bam"), (0x200, "qcfail", "flags.bam"),
                             (0x400, "dup", "flags.bam"), (0x4, "unmap", "flags.bam"),
                             (0x800, "supp", "supplementary.bam")]:
        for r in base:
            x = pysam.AlignedSegment.fromstring(r.to_string(), pysam.AlignmentHeader.from_dict(header))
            x.query_name = r.query_name + "_" + tag
            x.flag = r.flag | bit
            outs[target].append(x)
    for name, reads in outs.items():
        path = outdir / name
        tmp = str(path) + ".tmp.bam"
        with pysam.AlignmentFile(tmp, "wb", header=header) as out:
            for r in reads:
                out.write(r)
        sh("samtools sort -o %s %s && rm %s && samtools index %s" % (path, tmp, tmp, path))
    return (c, mid - 150, mid + 150)


def write_sites(outdir, windows, vcf_path, ref_lengths, flag_region):
    rng = random.Random(SEED + 1)
    covered = [w for w in windows if "zero coverage" not in w[3]]
    zero = [w for w in windows if "zero coverage" in w[3]][0]

    def dump(name, rows):
        with open(outdir / name, "w") as fh:
            for row in rows:
                fh.write(row if isinstance(row, str) else "\t".join(map(str, row)))
                fh.write("\n")

    snv, indel = [], []
    for rec in pysam.VariantFile(vcf_path):
        for c, s, e, _ in covered:
            if rec.chrom == c and s <= rec.pos - 1 < e:
                cls = classify(rec)
                if cls and cls.startswith("snv"):
                    snv.append((c, rec.pos, rec.pos))
                elif cls:
                    indel.append((c, rec.pos, rec.pos))
                    indel.append((c, rec.pos + 1, rec.pos + 1))
    dump("sites_snv.txt", snv)
    dump("sites_indel.txt", sorted(set(indel), key=lambda x: (x[0] != "chr6", x[1])))

    # dense: every base of the central 1 kb of the first het SNV window on chr17
    dw = [w for w in covered if w[0] == "chr17" and w[3].startswith("snv_het")][0]
    mid = (dw[1] + dw[2]) // 2
    dense = [(dw[0], p, p) for p in range(mid - 499, mid + 501)]
    dump("sites_dense.txt", dense)
    dump("sites_dense_range.txt", [(dw[0], dense[0][1], dense[-1][2])])
    shuffled = dense[:]
    rng.shuffle(shuffled)
    dump("sites_shuffled.txt", shuffled)
    dump("sites_reversed.txt", dense[::-1])

    # interleave: alternate chr17 / chr6 / chr17 ... using SNV sites and dense
    c6 = [s for s in snv if s[0] == "chr6"]
    c17 = [s for s in snv if s[0] == "chr17"] + dense[::50]
    inter = []
    for i in range(max(len(c6), len(c17))):
        if i < len(c17):
            inter.append(c17[i])
        if i < len(c6):
            inter.append(c6[i])
    dump("sites_interleaved.txt", inter)

    dupes = []
    for s in dense[100:120]:
        dupes += [s, s]
    dupes += dense[100:110]
    dump("sites_dupes.txt", dupes)

    a = dense[200][1]
    dump("sites_overlap.txt", [(dw[0], a, a + 50), (dw[0], a + 25, a + 75), (dw[0], a + 40, a + 45)])

    c6len = ref_lengths["chr6"]
    c17len = ref_lengths["chr17"]
    zmid = (zero[1] + zero[2]) // 2
    edge = [
        "# comment line",
        "",
        ("chr6", 1, 1),
        ("chr6", c6len, c6len),
        ("chr17", 1, 1),
        ("chr17", c17len, c17len),
        ("chr17", c17len + 10, c17len + 10),       # past the contig end
        ("chr6", zmid, zmid + 10),                 # zero coverage
        ("chrUn_notreal", 100, 100),              # unknown contig
        ("chr17", dense[10][1] + 5, dense[10][1]), # start > end
        "chr17\t%d" % dense[20][1],                # too few fields
        "chr17\tabc\tdef",                         # non-numeric
        ("chr17", dense[30][1], dense[30][1]),     # a valid, covered site in the middle
        "chr17 %d %d" % (dense[40][1], dense[40][1]),  # space separated
    ]
    dump("sites_edge.txt", edge)

    # large: every 3rd base across all covered windows (~5k sites)
    large = []
    for c, s, e, _ in covered:
        large += [(c, p, p) for p in range(s + 1, e + 1, 3)]
    dump("sites_large.txt", large)

    fr = flag_region
    dump("sites_flags.txt", [(fr[0], fr[1] + 1, fr[2])])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cache", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    args = ap.parse_args()
    cache, out = args.cache, args.out
    out.mkdir(parents=True, exist_ok=True)

    bam_path = cache / "Exome_Norm_sorted_mrkdup_bqsr.bam"
    ref = cache / "ref_genome.fa"
    vcf = cache / "Exome_Norm_HC_calls.vcf"
    targets = read_bed(cache / "exome_regions.bed")

    bam = pysam.AlignmentFile(str(bam_path), index_filename=str(cache / "Exome_Norm_sorted_mrkdup_bqsr.bai"))
    windows = pick_windows(bam, str(vcf), targets)
    with open(out / "windows.bed", "w") as fh:
        fh.write("# chrom\tstart0\tend0\treason (generated by make_hcc1395_fixtures.py, seed %d)\n" % SEED)
        for c, s, e, why in windows:
            fh.write("%s\t%d\t%d\t%s\n" % (c, s, e, why))

    slice_bam = out / "exome_norm.slice.bam"
    write_slice(bam, windows, slice_bam)
    masked = out / "ref.masked.fa.gz"
    write_masked_reference(str(ref), str(slice_bam), windows, masked)

    # CRAM with the reference embedded so decoding does not depend on
    # finding the (bgzipped) reference; -f is still passed by the tests.
    sh("samtools view -C -T %s --output-fmt-option embed_ref=1 -o %s %s && samtools index %s"
       % (masked, out / "exome_norm.slice.cram", slice_bam, out / "exome_norm.slice.cram"))

    # two libraries, and the same with no LB at all
    rg_two = [{"ID": "rgA", "LB": "libA", "SM": "HCC1395BL_DNA", "PL": "ILLUMINA"},
              {"ID": "rgB", "LB": "libB", "SM": "HCC1395BL_DNA", "PL": "ILLUMINA"}]
    write_relabelled(str(slice_bam), str(out / "twolib.slice.bam"), rg_two, lambda r: lib_for(r.query_name))
    rg_nolb = [{"ID": "rgA", "SM": "HCC1395BL_DNA", "PL": "ILLUMINA"}]
    write_relabelled(str(slice_bam), str(out / "nolb.slice.bam"), rg_nolb, lambda r: "rgA")

    snv_window = [w for w in windows if w[3].startswith("snv_het") and w[0] == "chr17"][0]
    flag_region = write_flag_fixtures(str(slice_bam), snv_window, out)

    lengths = {c: bam.get_reference_length(c) for c in bam.references}
    write_sites(out, windows, str(vcf), lengths, flag_region)

    # an unindexed copy for the missing-index error test
    sh("cp %s %s" % (out / "base.bam", out / "noindex.bam"))

    with open(out / "SOURCES.md5", "w") as fh:
        for p in sorted(cache.iterdir()):
            if p.is_file() and p.suffix in (".bam", ".bai", ".vcf", ".bed", ".tar"):
                h = hashlib.md5()
                with open(p, "rb") as f:
                    for chunk in iter(lambda: f.read(1 << 20), b""):
                        h.update(chunk)
                fh.write("%s  %s\n" % (h.hexdigest(), p.name))

    total = sum(p.stat().st_size for p in out.rglob("*") if p.is_file() and "expected" not in p.parts)
    print("fixture total: %.2f MB" % (total / 1e6), file=sys.stderr)
    if total > SIZE_BUDGET:
        sys.exit("fixtures exceed the %d byte budget" % SIZE_BUDGET)


if __name__ == "__main__":
    main()
