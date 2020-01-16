#!/bin/bash

(cd vendor && \
    wget https://github.com/samtools/samtools/releases/download/1.10/samtools-1.10.tar.bz2 && \
    wget https://www.zlib.net/zlib-1.2.11.tar.gz && \
    wget https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz && \
    wget https://tukaani.org/xz/xz-5.2.4.tar.gz && \
    wget https://curl.haxx.se/download/curl-7.67.0.tar.gz && \
    wget https://tls.mbed.org/download/mbedtls-2.16.4-apache.tgz)
