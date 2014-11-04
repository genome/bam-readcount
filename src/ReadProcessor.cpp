#include "ReadProcessor.hpp"

#include "auxfields.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>

ReadProcessor::ReadProcessor(
          char const* seq_name
        , int ref_len
        , char** ref_pointer
        , bam_plbuf_t* pileup_buffer
        )
    : seq_name(seq_name)
    , ref_len(ref_len)
    , ref_pointer(ref_pointer)
    , pileup_buffer(pileup_buffer)
{
}

ReadProcessor::~ReadProcessor() {
    flush();
}

ReadState::ReadState(char const* seq_name, int ref_len, bam1_t const* b, char const* ref)
    : seq_name(seq_name)
    , ref_len(ref_len)
    , ref_pos(b->core.pos)
    , read_pos(0)
    , sum_of_mismatch_qualities(0)
    , last_mismatch_pos(-1)
    , last_mismatch_qual(0)
    , left_clip(0)
    , clipped_length(b->core.l_qseq)
    , right_clip(b->core.l_qseq)
    , seq(bam1_seq(b))
    , ref(ref)
    , b(b)
{
}

bool ReadState::process_match(int len) {
    for (int j = 0; j < len; ++j) {
        int current_base_position = read_pos + j;
        int read_base = bam1_seqi(seq, current_base_position);
        int refpos = ref_pos + j;
        int ref_base;
        if(ref_len && refpos > ref_len) {
            fprintf(stderr, "WARNING: Request for position %d in sequence %s is > length of %d!\n",
                refpos, seq_name, ref_len);
            return false;
        }
        ref_base = bam_nt16_table[(int)ref[refpos]];

        if(read_base != ref_base && ref_base != 15 && read_base != 0) {
            int qual = bam1_qual(b)[current_base_position];

            //mismatch, so store the qualities
            if(last_mismatch_pos != -1) {
                if(last_mismatch_pos + 1 != current_base_position) {
                    //not an adjacent mismatch
                    sum_of_mismatch_qualities += last_mismatch_qual;
                    last_mismatch_qual = qual;
                    last_mismatch_pos = current_base_position;
                }
                else {
                    if(last_mismatch_qual < qual) {
                        last_mismatch_qual = qual;
                    }
                    last_mismatch_pos = current_base_position;
                }
            }
            else {
                last_mismatch_pos = current_base_position;
                last_mismatch_qual = qual;
            }
        }
    }
    ref_pos += len;
    read_pos += len;

    return true;
}

void ReadState::process_del(int len) {
    ref_pos += len;
}

void ReadState::process_ins(int len) {
    read_pos += len;
}

void ReadState::process_soft_clip(int len) {
    if(read_pos == 0) {
        left_clip += len;
    }
    else {
        right_clip -= len;
    }

    read_pos += len;
    clipped_length -= len;

    if(clipped_length < 0) {
        std::cerr << "After removing the clipping the length is less than 0 for read " <<
            bam1_qname(b) << "\n";
    }
}

void ReadProcessor::push_read(bam1_t* b) {
    //retrieve reference
    char *ref = *ref_pointer;
    //FIXME Won't want to do this if refseq is not included

    //calculate single nucleotide mismatches and sum their qualities
    uint32_t* cigar = bam1_cigar(b);
    const bam1_core_t* core = &(b->core);

    ReadState state(seq_name, ref_len, b, ref);
    bool valid = true;
    for(int i = 0; valid && (i < core->n_cigar); ++i) {
        int op_length = cigar[i]>>4;
        int op = cigar[i]&0xf;
        switch (op) {
            case BAM_CMATCH:
                valid = state.process_match(op_length);
                break;

            case BAM_CDEL:
            case BAM_CREF_SKIP:
                state.process_del(op_length);
                break;

            case BAM_CINS:
                state.process_ins(op_length);
                break;

            case BAM_CSOFT_CLIP:
                state.process_soft_clip(op_length);
                break;
        }
    }
    //add in any remaining mismatch sums; should be 0 if no mismatch
    state.sum_of_mismatch_qualities += state.last_mismatch_qual;

    //inefficiently scan again to determine the distance in leftmost read coordinates to the first Q2 base
    int three_prime_index = -1;

    int q2_pos = -1;
    int k;
    int increment;
    uint8_t *qual = bam1_qual(b);


    if(core->flag & BAM_FREVERSE) {
        k = 0;
        three_prime_index = std::max(k, state.left_clip);
        increment = 1;
    }
    else {
        k = core->l_qseq - 1;
        three_prime_index = std::min(k, state.right_clip);
        increment = -1;
    }

    while(q2_pos < 0 && k >= 0 && k < core->l_qseq) {
        if(qual[k] != 2) {
            q2_pos = k-1;
            break;
        }
        k += increment;
    }
    if(core->flag & BAM_FREVERSE) {
        three_prime_index = std::max(three_prime_index, q2_pos);
    }
    else if (q2_pos != -1) {
        three_prime_index = std::min(three_prime_index, q2_pos);
    }

    //store the value on the read, we're assuming it is always absent.
    //This assumption may fail. Future proof if this idea has value
    aux_zm_t zm;
    zm.sum_of_mismatch_qualities = state.sum_of_mismatch_qualities;
    zm.clipped_length = state.clipped_length;
    zm.left_clip = state.left_clip;
    zm.three_prime_index = three_prime_index;
    zm.q2_pos = q2_pos;

    std::string zm_str = zm.to_string();
    bam_aux_append(b, "Zm", 'Z', zm_str.size() + 1,
            reinterpret_cast<uint8_t*>(&zm_str[0]));

    //This just pushes all reads into the pileup buffer
    bam_plbuf_push(b, pileup_buffer);
}

void ReadProcessor::flush() {
    if (pileup_buffer) {
        bam_plbuf_push(0, pileup_buffer);
        bam_plbuf_destroy(pileup_buffer);
        pileup_buffer = 0;
    }
}
