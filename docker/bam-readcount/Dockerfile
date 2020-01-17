FROM seqfu/libnss:latest

# tzdata config from https://stackoverflow.com/a/47909037
ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true

RUN apt-get update && \
    # Build tools  
    apt-get install -y build-essential && \
    # htslib requirements
    apt-get install -y cmake && \
    # Clean up
    apt-get clean all && \
    rm -rf /var/lib/apt/lists/* && rm -rf /tmp/* && rm -rf /var/tmp/*

COPY bam-readcount /bam-readcount

RUN cd /bam-readcount && \
    rm -rf build && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make

FROM seqfu/libnss:latest
COPY --from=0 /bam-readcount/build/bin/bam-readcount /bin/bam-readcount
CMD ["/bin/bam-readcount"]
