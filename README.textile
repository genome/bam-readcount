h1. bam-readcount 
!https://travis-ci.org/genome/bam-readcount.svg?branch=master!:https://travis-ci.org/genome/bam-readcount
!https://coveralls.io/repos/genome/bam-readcount/badge.svg?branch=master&service=github(Coverage Status)!:https://coveralls.io/github/genome/bam-readcount?branch=master

p. The purpose of this program is to generate metrics at single nucleotide positions.
There are number of metrics generated which can be useful for filtering out false positive calls.
Help is currently available on the commandline if you do not supply the program
any arguments.

bc. Usage: bam-readcount [OPTIONS] <bam_file> [region]
Generate metrics for bam_file at single nucleotide positions.
Example: bam-readcount -f ref.fa some.bam
Available options:
  -h [ --help ]                         produce this message
  -v [ --version ]                      output the version
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

This program reports readcounts for each base at each position requested.
It also reports the average base quality of these bases and mapping qualities of
the reads containing each base.

The list of regions should be formatted as chromosome start and end. Each field should be tab separated and coordinates should be 1-based.
The optional region specification on the command line should follow the same format as that used for samtools (chr:start-stop)

h3. Normal Output

The output format prints to standard out as follows:

bc. chr	position	reference_base	depth	base:count:avg_mapping_quality:avg_basequality:avg_se_mapping_quality:num_plus_strand:num_minus_strand:avg_pos_as_fraction:avg_num_mismatches_as_fraction:avg_sum_mismatch_qualities:num_q2_containing_reads:avg_distance_to_q2_start_in_q2_reads:avg_clipped_length:avg_distance_to_effective_3p_end   ...



Each tab separated field contains the following information, each one is tab-separated:

base -> the base that all reads following in this field contain at the reported position i.e. C

count -> the number of reads containing the base

avg_mapping_quality -> the mean mapping quality of reads containing the base

avg_basequality -> the mean base quality for these reads

avg_se_mapping_quality -> mean single ended mapping quality

num_plus_strand -> number of reads on the plus/forward strand

num_minus_strand -> number of reads on the minus/reverse strand

avg_pos_as_fraction -> average position on the read as a fraction (calculated with respect to the length after clipping). This value is normalized to the center of the read (bases occurring strictly at the center of the read have a value of 1, those occurring strictly at the ends should approach a value of 0)

avg_num_mismatches_as_fraction -> average number of mismatches on these reads per base

avg_sum_mismatch_qualities -> average sum of the base qualities of mismatches in the reads

num_q2_containing_reads -> number of reads with q2 runs at the 3' end

avg_distance_to_q2_start_in_q2_reads -> average distance of position (as fraction of unclipped read length) to the start of the q2 run

avg_clipped_length -> average clipped read length of reads

avg_distance_to_effective_3p_end -> average distance to the 3' prime end of the read (as fraction of unclipped read length)

h3. Per Library Output

Beginning with version 0.5.0, bam-readcount can now output counts per each library within a single BAM file. Libraries are reported with the same metrics as in
normal counting, but each library is listed by name and its metrics denoted by curly braces as follows:

bc. chr	position	reference_base	depth	library_1_name	{	base:count:avg_mapping_quality:avg_basequality:avg_se_mapping_quality:num_plus_strand:num_minus_strand:avg_pos_as_fraction:avg_num_mismatches_as_fraction:avg_sum_mismatch_qualities:num_q2_containing_reads:avg_distance_to_q2_start_in_q2_reads:avg_clipped_length:avg_distance_to_effective_3p_end	}   ...   library_N_name	{	base:count:avg_mapping_quality:avg_basequality:avg_se_mapping_quality:num_plus_strand:num_minus_strand:avg_pos_as_fraction:avg_num_mismatches_as_fraction:avg_sum_mismatch_qualities:num_q2_containing_reads:avg_distance_to_q2_start_in_q2_reads:avg_clipped_length:avg_distance_to_effective_3p_end	}

In the output, after the chromosome, position, reference base, and depth fields the of per library output is formated as lib_name[TAB]{bam readcount metrics}[TAB]lib_name[TAB]{bam readcount metrics}. The libraries are repeated in this format from library 1 to library N.

h2. Build Dependencies

* git
* cmake 2.8.3+ ("cmake.org":http://cmake.org)

h3. Available Packages for Dependencies
* For APT-based systems (Debian, Ubuntu), install the following packages:

bc. sudo apt-get install build-essential git-core cmake zlib1g-dev libncurses-dev patch

* For RPM-based systems (Fedora, CentOS, RHEL), install the following packages instead:

bc. sudo yum groupinstall "Development tools" 
sudo yum install zlib-devel ncurses-devel cmake patch

Note that for some RPM based systems (like RHEL) and older versions of Ubuntu, you will need to install cmake 2.8.3 or greater yourself as the packaged version is older.


h2. Build Instructions

h3. Clone the bam-readcount repository
* Recursively clone the git repository

bc.         git clone https://github.com/genome/bam-readcount.git

h3. Compile bam-readcount
* bam-readcount does not support in source builds. Create a new build directory, enter it, and run:

bc.         cmake /path/to/bam-readcount/repo
        cd /path/to/bam-readcount/repo
        make

p. The binary can then be found in the bin/ subdirectory of your build directory.

h2. FAQ

h3. I get errors from cmake about missing modules. How do I fix this?

These errors should no longer occur as of commit e48b05cbb9a9222d7585958f9704b147f0a4cbea. For earlier commits, bam-readcount contains a git submodule called build-common. It contains helper modules for cmake. If you downloaded the source as a tarball from github or forgot to do a recursive clone using git, then you will not have this submodule and will see cmake errors. If you are using git, we recommend you go back and use the --recursive option when cloning the repository. If you cannot use git, follow the instructions below to remedy the situation.

# Download the build-common module separately "here":https://github.com/genome/build-common/tarball/master
# Extract that tarball, and rename the directory it creates to 'build-common'.
# Replace the empty build-common subdirectory in the sniper directory with directory you just created.
# Resume following the build instructions.

h3. I get a segfault when attempting to run on a whole bam file.

This is a known bug that will be addressed in future versions. In most cases, you will only need read counts on a subset of positions and specifying the sites of interest is functional. Should you need to run the entire genome through then running by chromosome and concatenating the results should be a decent workaround (e.g. like bam-readcount -f ref.fasta some.bam chr1). 

h2. User Support

p. Please first search "Biostar":http://www.biostars.org and then ask a question there if needed.  We automatically monitor "Biostar":http://www.biostars.org for questions related to our tools.
