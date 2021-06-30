Vendored libraries
==================


Licenses
--------

See each package for license information.


URLs
----

https://github.com/samtools/samtools/releases/download/1.10/samtools-1.10.tar.bz2

https://www.zlib.net/zlib-1.2.11.tar.gz

https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz

https://tukaani.org/xz/xz-5.2.4.tar.gz

https://curl.haxx.se/download/curl-7.67.0.tar.gz

https://tls.mbed.org/download/mbedtls-2.16.4-apache.tgz

`boost-1.55-bamrc.tar.gz` is a custom subset of the Boost library.

`Makefile.disable_curl.patch` is not currently used, but can 
be enabled to disable building of `curl`.


Notes
-----

We chose `mbedtls-2.16.4-apache.tgz` for `libcurl` SSL support as it has
no additional dependencies. This is used (via `https`) to query the ENA
CRAM registry for reference hashes. 

Instead of `libcurl` another good option might be wolfSSL's tiny-cURL
distribution, which builds a lighter `curl` library.


