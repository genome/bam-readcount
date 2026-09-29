"""Parser for bam-readcount output lines.

Each line is
    chrom  pos  ref  depth  <allele fields...>
or, with -p (per library),
    chrom  pos  ref  depth  LIB  {  <allele fields...>  }  LIB2  {  ...  }

Each allele field is 14 ':'-separated values (see README "Output").
"""

from dataclasses import dataclass, field
from typing import Dict, List

STAT_NAMES = [
    "count",
    "avg_mapq",
    "avg_bq",
    "avg_se_mapq",
    "plus",
    "minus",
    "avg_pos_as_fraction",
    "avg_num_mismatches_as_fraction",
    "avg_sum_mismatch_qualities",
    "num_q2_reads",
    "avg_distance_to_q2_start",
    "avg_clipped_length",
    "avg_distance_to_3p_end",
]
INT_STATS = {"count", "plus", "minus", "num_q2_reads"}
BASES = ["=", "A", "C", "G", "T", "N"]


@dataclass
class AlleleStat:
    allele: str
    count: int
    avg_mapq: float
    avg_bq: float
    avg_se_mapq: float
    plus: int
    minus: int
    avg_pos_as_fraction: float
    avg_num_mismatches_as_fraction: float
    avg_sum_mismatch_qualities: float
    num_q2_reads: int
    avg_distance_to_q2_start: float
    avg_clipped_length: float
    avg_distance_to_3p_end: float

    @property
    def is_indel(self):
        return self.allele[0] in "+-"


@dataclass
class Record:
    chrom: str
    pos: int
    ref: str
    depth: int
    # library name -> ordered list of allele stats; "all" when not per-library
    libs: Dict[str, List[AlleleStat]] = field(default_factory=dict)
    raw: str = ""

    @property
    def key(self):
        return (self.chrom, self.pos)

    def alleles(self, lib="all"):
        return {a.allele: a for a in self.libs.get(lib, [])}

    def summed(self):
        """Allele -> (count, plus, minus) summed across libraries."""
        out = {}
        for stats in self.libs.values():
            for a in stats:
                c, p, m = out.get(a.allele, (0, 0, 0))
                out[a.allele] = (c + a.count, p + a.plus, m + a.minus)
        return out


def parse_allele(text):
    allele, rest = text.split(":", 1)
    vals = rest.split(":")
    if len(vals) != len(STAT_NAMES):
        raise ValueError("expected %d stats in %r, got %d" % (len(STAT_NAMES), text, len(vals)))
    kwargs = {}
    for name, v in zip(STAT_NAMES, vals):
        kwargs[name] = int(v) if name in INT_STATS else float(v)
    return AlleleStat(allele=allele, **kwargs)


def parse_line(line, per_lib=False):
    line = line.rstrip("\n")
    f = line.split("\t")
    if len(f) < 4:
        raise ValueError("short line: %r" % line)
    rec = Record(chrom=f[0], pos=int(f[1]), ref=f[2], depth=int(f[3]), raw=line)
    rest = f[4:]
    if not per_lib:
        rec.libs["all"] = [parse_allele(x) for x in rest]
        return rec
    i = 0
    while i < len(rest):
        lib = rest[i]
        if rest[i + 1] != "{":
            raise ValueError("expected '{' after library %r in %r" % (lib, line))
        j = rest.index("}", i + 2)
        rec.libs[lib] = [parse_allele(x) for x in rest[i + 2:j]]
        i = j + 1
    return rec


def parse(text, per_lib=False):
    return [parse_line(l, per_lib) for l in text.splitlines() if l.strip()]
