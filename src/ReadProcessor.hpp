#pragma once

#include <bam.h>

struct ReadState {
    char const* seq_name;
    int ref_len;
    int ref_pos;
    int read_pos;
    int sum_of_mismatch_qualities;
    int last_mismatch_pos;
    int last_mismatch_qual;
    int left_clip;
    int clipped_length;
    int right_clip;
    uint8_t* seq;
    char const* ref;
    bam1_t const* b;

    ReadState(char const* seq_name, int ref_len, bam1_t const* b, char const* ref);

    bool process_match(int len);
    void process_del(int len);
    void process_ins(int len);
    void process_soft_clip(int len);
};

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
