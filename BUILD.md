Build instructions
==================


Requirements
------------

    A C++ toolchain
    cmake 
    make

On a Debian Linux-based system such as Ubuntu 

    apt install build-essential cmake

will install the required software, or see `Build (Docker)` below.

Builds are currently failing under OS X: see `OS X` below.

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


Build (Docker-based)
--------------------

This is mainly useful when working on the `bam-readcount` code.
  
If you have Docker running, build using the included Docker image

    cd docker/minimal-cmake
    make interact

This will start a minimal Docker container with `build-essential` and
`cmake` installed, with this cloned repository mounted at 

    /bam-readcount

and starting in that directory. Then follow the build instructions 
above.


Build Docker image
------------------

This is a two-stage build, copying the `bam-readcount` binary from the
first stage (with build tools) into a minimal image as
`/bin/bam-readcount`. To run the build yourself

    cd docker/bam-readcount
    make build
    # Try out interactively
    make interact
    # Change the NAME at the top of the Makefile first!
    make push


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


OS X
----

OS X builds fail on our High Sierra machine and under GitHub Actions
with `macos-10.15` (Catalina) and `macos-11` (Big Sur).

When linking Boost, there are pages of errors.

This is also true of versions prior to the samtools-1.10 switch.


Todo
----

Add `URL_HASH` for vendored libraries for CMake verification.
