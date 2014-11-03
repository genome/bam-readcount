#include "ReadProcessor.hpp"

#include "auxfields.hpp"

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

void ReadProcessor::push_read(bam1_t* b) {
    //retrieve reference
    char *ref = *ref_pointer;
    //FIXME Won't want to do this if refseq is not included

    //calculate single nucleotide mismatches and sum their qualities
    uint8_t *seq = bam1_seq(b);
    uint32_t *cigar = bam1_cigar(b);
    const bam1_core_t *core = &(b->core);
    int i, reference_position, read_position;
    uint32_t sum_of_mismatch_qualities=0;
    int left_clip = 0;
    int clipped_length = core->l_qseq;
    int right_clip = core->l_qseq;

    int last_mismatch_position = -1;
    int last_mismatch_qual = 0;

    for(i = read_position = 0, reference_position = core->pos; i < core->n_cigar; ++i) {
        int j;
        int op_length = cigar[i]>>4;
        int op = cigar[i]&0xf;

        if(op == BAM_CMATCH) {
            for(j = 0; j < op_length; j++) {
                int current_base_position = read_position + j;
                int read_base = bam1_seqi(seq, current_base_position);
                int refpos = reference_position + j;
                int ref_base;
                if(ref_len && refpos > ref_len) {
                    fprintf(stderr, "WARNING: Request for position %d in sequence %s is > length of %d!\n",
                        refpos, seq_name, ref_len);
                    continue;
                }
                ref_base = bam_nt16_table[(int)ref[refpos]];

                if(ref[refpos] == 0) break; //out of bounds on reference
                if(read_base != ref_base && ref_base != 15 && read_base != 0) {
                    //mismatch, so store the qualities
                    int qual = bam1_qual(b)[current_base_position];
                    if(last_mismatch_position != -1) {
                        if(last_mismatch_position + 1 != current_base_position) {
                            //not an adjacent mismatch
                            sum_of_mismatch_qualities += last_mismatch_qual;
                            last_mismatch_qual = qual;
                            last_mismatch_position = current_base_position;
                        }
                        else {
                            if(last_mismatch_qual < qual) {
                                last_mismatch_qual = qual;
                            }
                            last_mismatch_position = current_base_position;
                        }
                    }
                    else {
                        last_mismatch_position = current_base_position;
                        last_mismatch_qual = qual;
                    }
                }
            }
            if(j < op_length) break;
            reference_position += op_length;
            read_position += op_length;
        } else if(op == BAM_CDEL || op == BAM_CREF_SKIP) {  //ignoring indels
            reference_position += op_length;
        } else if(op ==BAM_CINS) { //ignoring indels
            read_position += op_length;
        }
        else if(op == BAM_CSOFT_CLIP) {
            read_position += op_length;

            clipped_length -= op_length;
            if(i == 0) {
                left_clip += op_length;
            }
            else {
                right_clip -= op_length;
            }
            if(clipped_length < 0) {
                fprintf(stderr, "After removing the clipping the length is less than 0 for read %s\n",bam1_qname(b));
            }
        }
    }
    //add in any remaining mismatch sums; should be 0 if no mismatch
    sum_of_mismatch_qualities += last_mismatch_qual;

    //inefficiently scan again to determine the distance in leftmost read coordinates to the first Q2 base
    int three_prime_index = -1;
    int q2_pos = -1;
    int k;
    int increment;
    uint8_t *qual = bam1_qual(b);
    if(core->flag & BAM_FREVERSE) {
        k = three_prime_index = 0;
        increment = 1;
        if(three_prime_index < left_clip) {
            three_prime_index = left_clip;
        }
    }
    else {
        k = three_prime_index = core->l_qseq - 1;
        increment = -1;
        if(three_prime_index > right_clip) {
            three_prime_index = right_clip;
        }

    }
    while(q2_pos < 0 && k >= 0 && k < core->l_qseq) {
        if(qual[k] != 2) {
            q2_pos = k-1;
            break;
        }
        k += increment;
    }
    if(core->flag & BAM_FREVERSE) {
        if(three_prime_index < q2_pos) {
            three_prime_index = q2_pos;
        }
    }
    else {
        if(three_prime_index > q2_pos && q2_pos != -1) {
            three_prime_index = q2_pos;
        }
    }
    uint8_t temp[5*4+1];
    temp[5*4]=0;
    memcpy(temp, &sum_of_mismatch_qualities,4);
    memcpy(temp+4, &clipped_length,4);
    memcpy(temp+8, &left_clip,4);
    memcpy(temp+12, &three_prime_index,4);
    memcpy(temp+16, &q2_pos,4);

    //store the value on the read, we're assuming it is always absent. This assumption may fail. Future proof if this idea has value
    aux_zm_t zm;
    zm.sum_of_mismatch_qualities = sum_of_mismatch_qualities;
    zm.clipped_length = clipped_length;
    zm.left_clip = left_clip;
    zm.three_prime_index = three_prime_index;
    zm.q2_pos = q2_pos;
    std::string zm_str = zm.to_string();
    bam_aux_append((bam1_t *)b, "Zm", 'Z', zm_str.size() + 1, (uint8_t*)&zm_str[0]);

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
