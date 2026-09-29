"""The golden-output matrix shared by test_golden.py and update_golden.py.

Each case is (name, args); args are relative to test-data/hcc1395 and the
expected stdout lives in test-data/hcc1395/expected/<name>.tsv.gz.
"""

REF = "ref.masked.fa.gz"
BAM = "exome_norm.slice.bam"
CRAM = "exome_norm.slice.cram"
TWOLIB = "twolib.slice.bam"
NOLB = "nolb.slice.bam"

SITE_LISTS = ["snv", "indel", "dense", "edge"]

OPTION_SETS = {
    "default": [],
    "q1": ["-q", "1"],
    "q20": ["-q", "20"],
    "q60": ["-q", "60"],
    "b13": ["-b", "13"],
    "b30": ["-b", "30"],
    "q20b20": ["-q", "20", "-b", "20"],
    "i": ["-i"],
    "p": ["-p"],
    "pi": ["-p", "-i"],
}

# high-depth exon and a SNV pair used for positional-region cases
HIGH_DEPTH_REGION = "chr17:62231473-62231573"
SNV_REGIONS = ["chr6:112176257-112176257", "chr17:47840239-47840239"]


def _cases():
    cases = []
    for opt, flags in OPTION_SETS.items():
        for sites in SITE_LISTS:
            cases.append(("bam_%s_%s" % (opt, sites),
                          ["-w", "0"] + flags + ["-f", REF, "-l", "sites_%s.txt" % sites, BAM]))
    for opt in ["default", "i", "q20b20"]:
        for sites in ["snv", "indel"]:
            cases.append(("cram_%s_%s" % (opt, sites),
                          ["-w", "0"] + OPTION_SETS[opt] + ["-f", REF, "-l", "sites_%s.txt" % sites, CRAM]))
    for opt in ["default", "p", "pi"]:
        for sites in ["snv", "indel", "dense"]:
            cases.append(("twolib_%s_%s" % (opt, sites),
                          ["-w", "0"] + OPTION_SETS[opt] + ["-f", REF, "-l", "sites_%s.txt" % sites, TWOLIB]))
    cases.append(("nolb_default_snv", ["-w", "0", "-f", REF, "-l", "sites_snv.txt", NOLB]))
    for opt in ["default", "i"]:
        cases.append(("bam_%s_large" % opt,
                      ["-w", "0"] + OPTION_SETS[opt] + ["-f", REF, "-l", "sites_large.txt", BAM]))
    for name, flags in [("default", []), ("d10", ["-d", "10"]), ("d50", ["-d", "50"]),
                        ("q20", ["-q", "20"])]:
        cases.append(("region_highdepth_%s" % name, ["-w", "0"] + flags + ["-f", REF, BAM, HIGH_DEPTH_REGION]))
    cases.append(("region_snv_pair", ["-w", "0", "-f", REF, BAM] + SNV_REGIONS))
    return cases


CASES = _cases()
