samtools-1.10 refactor
======================


Caveats
-------

On my OS X machine (High Sierra), builds fail due to Boost. This is also
true of the current `genome/bam-readcount` `master`.

Builds were also failing under OS X with libcurl errors. The current
CMake configuration includes a patch for the htslib Makefile that is
applied by CMake that disables libcurl. There is currently a protocol 
warning when running `bam-readcount` that may be due to the absence of
libcurl (see `Todo` below).


Build
-----


### Download vendored libraries

Clone `samtools-1.10` branch of forked `genome/bam-readcount`

    git clone -b samtools-1.10 https://github.com/seqfu/bam-readcount
    cd bam-readcount

Already under `vendor/` are
  
    vendor/
      boost-1.55-bamrc.tar.gz
      Makefile.disable_curl.patch

Except for Boost the vendored libraries are not included in the
repository. To fetch them, run 

    # wget is required for this script
    0/populate_vendor.sh

This should download

    vendor/
      bzip2-1.0.8.tar.gz
      samtools-1.10.tar.bz2
      xz-5.2.4.tar.gz
      zlib-1.2.11.tar.gz


### Run Docker container

If you have Docker running, build using the included Docker image

    cd docker/minimal_cmake
    make interact

This will start a minimal docker container with `build-essential` and
`cmake` installed, with the cloned repository mounted at 

    /bam-readcount

and starting in that directory. 


### Build `bam-readcount` and vendored libraries

Make a build directory

    mkdir build

Run CMake from inside it

    cd build
    cmake ..

Run Make

    make 

This will build all the vendored libraries as well as `bam-readcount`.
The final binary will be

    bin/bam-readcount


### Test data

There is a small CRAM file under

    test-data/twolib.sorted.cram  

with associated reference

    test-data/rand1k.fa


Todo
----

Tested against `genome/bam-readcount` `master` with a simple BAM file 
converted to CRAM with identical output, but needs more testing. The
CRAM test did throw a warning; I will rerun it and add that here.

`find_library_names()` line 91 in 

    src/exe/bam-readcount/bamreadcount.cpp

has been modified to return an empty list because `sam_header2key_val`
has been removed (along with all of `sam_header.h`) and there is a new
(`hrecs`?) API to access header data that we will need to use. Enabling
`-p` still appears to work. It looks like the list is only used to print
expected library names to `STDERR`.

CRAM files will use the reference encoded in their header. We may want
to propagate the command-line-specified reference (and maybe make it
optional) as the reference.

We might want to reenable `libcurl` support since OS X builds are
breaking anyway. It is in the CMakeFiles but is commented out and should
be easy to restore. It is a little slow to compile but builds fine under
the Docker image. This might be why we get the warning

  [W::find_file_url] Failed to open reference "https://www.ebi.ac.uk/ena/cram/md5/11e5d1f36a8e123feb3dd934cc05569a": Protocol not supported

Add `URL_HASH` for vendored libraries for CMake verification.


