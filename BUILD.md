Build instructions
==================


Requirements
------------

    A C++ toolchain
    cmake 
    make

On a Debian Linux-based system such as Ubuntu 

    apt install build-essential cmake

will install the required software.

Builds are currently failing under OS X.

All required libraries are included in the repository under `vendor/`.
See [vendor/README.md](vendor/README.md) for more information.


Build
-----

Make a build directory

    mkdir build

Run CMake from inside it

    cd build
    cmake ..

Run Make

    make 

This will build all the vendored libraries as well as `bam-readcount`.
The final binary, which can be moved anywhere, is

    bin/bam-readcount

Try it on a test CRAM

    cd ../test-data
    ../build/bin/bam-readcount -f rand1k.fa twolib.sorted.cram


Test data
---------

There is a small two-library test CRAM file

    test-data/twolib.sorted.cram  

with associated reference

    test-data/rand1k.fa

The reference is encoded in the CRAM as 

    @SQ	SN:rand1k	LN:1000	M5:11e5d1f36a8e123feb3dd934cc05569a	UR:rand1k.fa

so `bam-readcount` should be run inside the `test-data` directory to
find the reference.


