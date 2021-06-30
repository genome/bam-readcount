bam-readcount
=============

![tests](https://github.com/seqfu/bam-readcount/actions/workflows/tests.yml/badge.svg?branch=actions-config)
![coverage](https://coveralls.io/repos/seqfu/bam-readcount/badge.svg?branch=actions-config&service=github)


`bam-readcount` runs on a `BAM` or `CRAM` file and generates metrics at
single nucleotide positions. These metrics can be useful for filtering
out false positive variant calls.

For support, please [search
`bam-readcount`](https://www.biostars.org/post/search/?query=bam-readcount) on
[Biostars](https://www.biostars.org) as many of the most frequently asked
questions about `bam-readcount` have been answered there.


Build
-----

Requires a C++ toolchain and `cmake`. For details see
[BUILD.md](BUILD.md). 

    mkdir build
    cd build
    cmake ..
    make
    # Executable is
    bin/bam-readcount


Usage
-----

Run with no arguments for command-line help:

    $ bam-readcount

    Usage: bam-readcount [OPTIONS] <bam_file> [region]
    Generate metrics for bam_file at single nucleotide positions.
    Example: bam-readcount -f ref.fa some.bam

    Available options:
      -h [ --help ]                         produce this message
      -v [ --version ]                      output the version number
      -q [ --min-mapping-quality ] arg (=0) minimum mapping quality of reads used
                                            for counting.
      -b [ --min-base-quality ] arg (=0)    minimum base quality at a position to
                                            use the read for counting.
      -d [ --max-count ] arg (=10000000)    max depth to avoid excessive memory
                                            usage.
      -l [ --site-list ] arg                file containing a list of regions to
                                            report readcounts within.
      -f [ --reference-fasta ] arg          reference sequence in the fasta format.
      -D [ --print-individual-mapq ] arg    report the mapping qualities as a comma
                                            separated list.
      -p [ --per-library ]                  report results by library.
      -w [ --max-warnings ] arg             maximum number of warnings of each type
                                            to emit. -1 gives an unlimited number.
      -i [ --insertion-centric ]            generate indel centric readcounts.
                                            Reads containing insertions will not be
                                            included in per-base counts

The optional `[region]` should be in the same format as `samtools`:

    chromosome:start-stop

The optional `-l` (`--site-list`) file should be tab-separated, no
header, one region per line:

    chromosome	start	end


### CRAM support

`bam-readcount` now supports CRAM files as input. 

If a reference is specified with `-f`, it will override whatever is in
the CRAM header.

Otherwise, the reference(s) encoded in the CRAM header or a lookup by
MD5 at ENA will be used.


Output
------

Output is tab-separated with no header to `STDOUT`, one line per
position:

    chr	position	reference_base	depth	base:count:avg_mapping_quality:avg_basequality:avg_se_mapping_quality:num_plus_strand:num_minus_strand:avg_pos_as_fraction:avg_num_mismatches_as_fraction:avg_sum_mismatch_qualities:num_q2_containing_reads:avg_distance_to_q2_start_in_q2_reads:avg_clipped_length:avg_distance_to_effective_3p_end   ...

There is one set of `:`-separated fields for each reported `base` with
statistics on the set of reads containing that base:

Field | Description
----- | -----------
base | The base, eg `C`
count | Number of reads
avg_mapping_quality | Mean mapping quality
avg_basequality | Mean base quality
avg_se_mapping_quality | Mean single ended mapping quality
num_plus_strand | Number of reads on the plus/forward strand
num_minus_strand | Number of reads on the minus/reverse strand
avg_pos_as_fraction | Average position on the read as a fraction, calculated with respect to the length after clipping. This value is normalized to the center of the read: bases occurring strictly at the center of the read have a value of 1, those occurring strictly at the ends should approach a value of 0
avg_num_mismatches_as_fraction | Average number of mismatches on these reads per base
avg_sum_mismatch_qualities | Average sum of the base qualities of mismatches in the reads
num_q2_containing_reads | Number of reads with q2 runs at the 3’ end
avg_distance_to_q2_start_in_q2_reads | Average distance of position (as fraction of unclipped read length) to the start of the q2 run
avg_clipped_length | Average clipped read length
avg_distance_to_effective_3p_end | Average distance to the 3’ prime end of the read (as fraction of unclipped read length)


### Per-library output

With the `-p` option, each output line will have a set of `{}`-delimited
results, one for each library:

    chr	position	reference_base	depth	library_1_name	{	base:count:avg_mapping_quality:avg_basequality:avg_se_mapping_quality:num_plus_strand:num_minus_strand:avg_pos_as_fraction:avg_num_mismatches_as_fraction:avg_sum_mismatch_qualities:num_q2_containing_reads:avg_distance_to_q2_start_in_q2_reads:avg_clipped_length:avg_distance_to_effective_3p_end	}   ...   library_N_name	{	base:count:avg_mapping_quality:avg_basequality:avg_se_mapping_quality:num_plus_strand:num_minus_strand:avg_pos_as_fraction:avg_num_mismatches_as_fraction:avg_sum_mismatch_qualities:num_q2_containing_reads:avg_distance_to_q2_start_in_q2_reads:avg_clipped_length:avg_distance_to_effective_3p_end	}


Bugs
----

### I get a segfault when attempting to run on a whole bam file

This is a known bug that will be addressed in future versions. In most
cases, you will only need read counts on a subset of positions and can
use `[region]` or `-l` to specify the sites of interest. Should you need
counts on an entire genome, a workaround is to run `bam-readcount` by
chromosome and then concatenate the results (e.g. `bam-readcount -f
ref.fasta some.bam chr1`).





