#pragma once

#include <bam.h>

class ReadProcessor {
public:
    ReadProcessor(
          char const* seq_name
        , int ref_len
        , char** ref_pointer
        , bam_plbuf_t* pileup_buffer
        );

    ~ReadProcessor();

    void push_read(bam1_t* b);
    void flush();

private:
    const char* seq_name;
    int ref_len;
    char **ref_pointer;
    bam_plbuf_t* pileup_buffer;
};
