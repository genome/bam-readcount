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

RUN apt-get update && \
    apt-get install -y python-minimal && \
    # Clean up
    apt-get clean all && \
    rm -rf /var/lib/apt/lists/* && rm -rf /tmp/* && rm -rf /var/tmp/*

RUN apt-get update && \
    apt-get install -y lcov && \
    # Clean up
    apt-get clean all && \
    rm -rf /var/lib/apt/lists/* && rm -rf /tmp/* && rm -rf /var/tmp/*

