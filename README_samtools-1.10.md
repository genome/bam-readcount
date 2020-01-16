samtools-1.10 refactor
======================

This branch is a refactor of `bam-readcount` to the `samtools-1.10` 
(and `htslib-1.10`) API, which adds CRAM support. Should build in a 
minimal environment with C and C++ compilers, Make, and CMake.


Build
-----

Builds are failing under OS X, see `OS X` below.

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
      mbedtls-2.16.4-apache.tgz
      samtools-1.10.tar.bz2
      xz-5.2.4.tar.gz
      zlib-1.2.11.tar.gz

I chose `mbedtls-2.16.4-apache.tgz` for `libcurl` SSL support as it has
no additional dependencies. This is used (via `https`) to query the ENA
CRAM registry for reference hashes. Another option might be wolfSSL's
tiny-cURL distribution, which builds a lighter cURL with HTTPS support,
but the download process involves a form.


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

There is a small two-library test CRAM file

    test-data/twolib.sorted.cram  

with associated reference

    test-data/rand1k.fa

The reference is encoded in the CRAM as 

    @SQ	SN:rand1k	LN:1000	M5:11e5d1f36a8e123feb3dd934cc05569a	UR:rand1k.fa

so `bam-readcount` should be run inside the `test-data` directory to
find the reference.


Todo
----

Tested against `genome/bam-readcount` `master` with a simple BAM file 
converted to CRAM with identical output, but needs more testing.

`find_library_names()` line 91 in 

    src/exe/bam-readcount/bamreadcount.cpp

has been modified to return an empty list because `sam_header2key_val`
has been removed (along with all of `sam_header.h`) and there is a new
(`hrecs`?) API to access header data that we will need to use. Enabling
`-p` still appears to work. It looks like the list is only used to print
expected library names to `STDERR`.

CRAM files will use the reference encoded in their header (or found by
query to ENA CRAM registry, though I haven't tested that). We may want
to propagate the command-line-specified reference (and maybe make it
optional and use the CRAM-header reference as a default) as the
reference. 

Add `URL_HASH` for vendored libraries for CMake verification.

Lower CMake minimum version requirement in `BuildSamtools.cmake`, 
and be consistent throughout.


OS X
----

OS X builds fail for (at least) two reasons on my High Sierra machine.
This may be due to my setup with MacPorts etc; I haven't looked into
this much and the observations below might not be relevant.

Boost: 

When linking, I get pages of errors.

This is also true of the current `genome/bam-readcount` `master`.

cURL: 

Not sure what goes wrong here, Make just exits with

    make: *** [all] Error 2

I disabled `libcurl` for a while using the Makefile patch under

    vendor/Makefile.disable_curl.patch

Also getting errors 

    bam-readcount/src/lib/bamrc/BasicStat.hpp:5:10: fatal error:
          'sam.h' file not found
    #include "sam.h"
             ^~~~~~~
    1 error generated.

Not sure why, since it seems to be a build configuration problem but
doesn't happen in the minimal Docker build container.




