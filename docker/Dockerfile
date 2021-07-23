FROM ubuntu:focal

ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true

# Requirements
RUN apt-get update && \
    # Required build tools  
    apt-get install -y build-essential cmake && \
    # Need git to fetch the repository
    apt-get install -y git && \
    # Clean up
    apt-get clean all && \
    rm -rf /var/lib/apt/lists/* && rm -rf /tmp/* && rm -rf /var/tmp/*

# Clone and build v1.0.0
RUN cd / && \
    git clone https://github.com/genome/bam-readcount && \
    cd bam-readcount && \
    # For a specific tag enable the git checkout below
    git checkout v1.0.0 && \
    rm -rf build && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make


FROM ubuntu:focal

ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true

LABEL \
    description="Ubuntu Focal with bam-readcount v1.0.0"

RUN apt-get update

RUN mkdir -p /opt/bam-readcount/bin
WORKDIR /opt/bam-readcount
COPY --from=0 /bam-readcount/build/bin/bam-readcount /opt/bam-readcount/bin/bam-readcount
RUN ln -s /opt/bam-readcount/bin/bam-readcount /usr/bin/bam-readcount
