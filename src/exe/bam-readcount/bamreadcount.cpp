#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include "bamrc/auxfields.hpp"
#include "bamrc/ReadWarnings.hpp"

#include <stdio.h>
#include <memory>
#include <string.h>
#include "sam.h"
#include "faidx.h"
#include "khash.h"
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>


typedef char *str_p;
KHASH_MAP_INIT_STR(s, int)
KHASH_MAP_INIT_STR(r2l, str_p)

typedef struct {
    unsigned int read_count;    //number of reads containing the indel
    unsigned int sum_map_qualities; //sum of the mapping qualities of reads containing the indel
    unsigned int sum_single_ended_map_qualities; //sum of the single ended mapping qualities;
    unsigned int *mapping_qualities;
    unsigned int num_mapping_qualities;
    unsigned int num_plus_strand;
    unsigned int num_minus_strand;
    float sum_indel_location;
    float sum_q2_distance;
    unsigned int num_q2_reads;
    float sum_number_of_mismatches;
    unsigned int sum_of_mismatch_qualities;
    unsigned int sum_of_clipped_lengths;
    float sum_3p_distance;
    float *distances_to_3p;
    unsigned int num_distances_to_3p;
} indel_stat_t;
KHASH_MAP_INIT_STR(indels, indel_stat_t)


//Struct to store info to be passed around
typedef struct {
    faidx_t *fai;       //index into fasta file
    int tid;            //reference id
    char *ref;          //reference sequence
    int min_mapq;       //minimum mapping qualitiy to use
    int min_bq;       //minimum mapping qualitiy to use
    int beg,end;        //start and stop of region
    int len;            //length of currently loaded reference sequence
    int max_cnt;       //maximum depth to set on the pileup buffer 
    samfile_t *in;      //bam file
    int distribution;   //whether or not to display all mapping qualities
} pileup_data_t;

//struct to store reference for passing to fetch func
typedef struct {
    const char* seq_name;
    int ref_len;
    char **ref_pointer;
    bam_plbuf_t* pileup_buffer;
} fetch_data_t;

static std::auto_ptr<ReadWarnings> WARN;

// callback for bam_fetch()
static int fetch_func(const bam1_t *b, void *data) {
    //retrieve reference
    fetch_data_t* fetch_data = (fetch_data_t*) data;
    char *ref = *(fetch_data->ref_pointer);
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
                if(fetch_data->ref_len && refpos > fetch_data->ref_len) {
                    fprintf(stderr, "WARNING: Request for position %d in sequence %s is > length of %d!\n",
                        refpos, fetch_data->seq_name, fetch_data->ref_len);
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
    bam_plbuf_t *buf = fetch_data->pileup_buffer;
    bam_plbuf_push(b, buf);
    return 0;
}

/* This will convert all iub codes in the reads to N */
char const* bam_canonical_nt_table = "=ACGTN";
unsigned char bam_nt16_canonical_table[16] = { 0,1,2,5,
    3,5,5,5,
    4,5,5,5,
    5,5,5,5};

// callback for bam_plbuf_init()
// TODO allow for a simplified version that calculates less
static int pileup_func(uint32_t tid, uint32_t pos, int n, const bam_pileup1_t *pl, void *data)
{
    pileup_data_t *tmp = (pileup_data_t*)data;

    if ((int)pos >= tmp->beg && (int)pos < tmp->end) {

        //set up data structures to count bases
        unsigned char possible_calls = (unsigned char) strlen(bam_canonical_nt_table);
        unsigned int *read_counts = (unsigned int*)calloc(possible_calls,sizeof(unsigned int));
        unsigned int *sum_base_qualities = (unsigned int*)calloc(possible_calls,sizeof(unsigned int));
        unsigned int *sum_map_qualities = (unsigned int*)calloc(possible_calls,sizeof(unsigned int));
        unsigned int *sum_single_ended_map_qualities = (unsigned int*)calloc(possible_calls,sizeof(unsigned int));
        unsigned int **mapping_qualities = (unsigned int**)calloc(possible_calls, sizeof(unsigned int*));
        unsigned int *num_mapping_qualities = (unsigned int*)calloc(possible_calls, sizeof(unsigned int));
        unsigned int *num_plus_strand = (unsigned int*)calloc(possible_calls, sizeof(unsigned int));
        unsigned int *num_minus_strand = (unsigned int*)calloc(possible_calls, sizeof(unsigned int));
        float *sum_base_location = (float*)calloc(possible_calls, sizeof(float));
        float *sum_q2_distance = (float*)calloc(possible_calls, sizeof(float));
        unsigned int *num_q2_reads = (unsigned int*)calloc(possible_calls, sizeof(unsigned int));
        float *sum_number_of_mismatches = (float*)calloc(possible_calls,sizeof(float));
        unsigned int *sum_of_mismatch_qualities = (unsigned int*)calloc(possible_calls, sizeof(unsigned int));
        unsigned int *sum_of_clipped_lengths = (unsigned int*)calloc(possible_calls, sizeof(unsigned int));
        float *sum_3p_distance = (float*)calloc(possible_calls, sizeof(float));
        float **distances_to_3p = (float**)calloc(possible_calls, sizeof(float*));
        unsigned int *num_distances_to_3p = (unsigned int*)calloc(possible_calls, sizeof(unsigned int));

        int mapq_n = 0; //this tracks the number of reads that passed the mapping quality threshold

        //this is a hash to store indels
        khash_t(indels) *hash = kh_init(indels);

        //allocate enough mem to store relevant mapping qualities
        int i;
        for(i = 0; i < possible_calls; i++) {
            mapping_qualities[i] = (unsigned int*)calloc(n, sizeof(unsigned int));
            distances_to_3p[i] = (float*)calloc(n, sizeof(float));
        }

        //loop over the bases, recycling i here.
        for(i = 0; i < n; ++i) {
            const bam_pileup1_t *base = pl + i; //get base index
            if(!base->is_del && base->b->core.qual >= tmp->min_mapq && bam1_qual(base->b)[base->qpos] >= tmp->min_bq) {
                mapq_n++;

                //determine if we have an indel or not
                indel_stat_t dummy; //rather than do lots of test just do the indel math on this thing
                indel_stat_t *indel_stat = &dummy;
                if(base->indel != 0 && tmp->ref) {
                    //indel containing read exists here
                    //will need to
                    char *allele = (char*)calloc(abs(base->indel)+2,sizeof(char));   //allocate the allele string
                    if(base->indel > 0) {
                        allele[0] = '+';
                        int indel_base;
                        for(indel_base = 0; indel_base < base->indel; indel_base++) {
                            //scan indel allele off the read
                            allele[indel_base+1] = bam_canonical_nt_table[bam_nt16_canonical_table[bam1_seqi(bam1_seq(base->b), base->qpos + 1 + indel_base)]];
                        }
                        allele[indel_base+1] = '\0';  //null terminate the string

                    }
                    else {
                        //deletion
                        allele[0] = '-';
                        int indel_base;
                        for(indel_base = 0; indel_base < abs(base->indel); indel_base++) {
                            //scan indel allele off the reference
                            //FIXME this will break with no reference
                            allele[indel_base+1] = tmp->ref[pos + indel_base + 1];
                        }
                        allele[indel_base + 1] = '\0';  //null terminate the string

                    }
                    //fprintf(stderr,"Found read with indel allele %s at %d\n",allele,pos+1);
                    khiter_t indel = kh_get(indels,hash,allele);
                    if(indel == kh_end(hash)) {
                        //need to create a new entry
                        indel_stat_t new_indel;
                        new_indel.read_count = 0;
                        new_indel.sum_map_qualities = 0;
                        new_indel.sum_single_ended_map_qualities = 0;
                        new_indel.mapping_qualities = NULL;
                        new_indel.num_mapping_qualities = 0;
                        new_indel.num_plus_strand = 0;
                        new_indel.num_minus_strand = 0;
                        new_indel.sum_indel_location = 0.0;
                        new_indel.sum_q2_distance = 0.0;
                        new_indel.num_q2_reads = 0.0;
                        new_indel.sum_number_of_mismatches = 0.0;
                        new_indel.sum_of_mismatch_qualities = 0;
                        new_indel.sum_of_clipped_lengths = 0.0;
                        new_indel.sum_3p_distance = 0.0;
                        new_indel.distances_to_3p = NULL;
                        new_indel.num_distances_to_3p = 0;

                        //new_indel.
                        int put_error;
                        indel = kh_put(indels, hash,allele,&put_error);    //TODO should check for error here
                        kh_value(hash,indel) = new_indel;
                    }
                    else {
                        //found the site and we can free the allele string
                        free(allele);
                    }
                    indel_stat = &(kh_value(hash, indel));
                }
                indel_stat->read_count++;
                indel_stat->sum_map_qualities += base->b->core.qual;

                //the following are done regardless of whether or not there is an indel
                int c = (int) bam_nt16_canonical_table[bam1_seqi(bam1_seq(base->b), base->qpos)];   //convert to index
                read_counts[c] ++; //calloc should 0 out the mem
                sum_base_qualities[c] += bam1_qual(base->b)[base->qpos];
                sum_map_qualities[c] += base->b->core.qual;
                //add in strand info
                //TODO STORE THIS TO avoid repetitively calculating for indels
                if(base->b->core.flag & BAM_FREVERSE) {
                    //mapped to the reverse strand
                    num_minus_strand[c]++;
                    indel_stat->num_minus_strand++;
                }
                else {
                    //must be mapped to the plus strand
                    num_plus_strand[c]++;
                    indel_stat->num_plus_strand++;
                }

                int32_t left_clip = 0;
                int32_t clipped_length = base->b->core.l_qseq;
                int32_t mismatch_sum = 0;
                int32_t q2_val = 0;
                int32_t three_prime_index = 0;

                //hopefully grab out our calculated per/read values
                //FIXME these will be unavailable if there is no reference
                //TODO Make sure the defaults on the above are reasonable if there is nothing available
                uint8_t *tag_ptr = bam_aux_get(base->b, "Zm") + 1;
                if(tag_ptr) {
                    aux_zm_t zm = aux_zm_t::from_string((char const*)tag_ptr);
                    mismatch_sum = zm.sum_of_mismatch_qualities;
                    clipped_length = zm.clipped_length;
                    left_clip = zm.left_clip;
                    three_prime_index = zm.three_prime_index;
                    q2_val = zm.q2_pos;

                    sum_of_mismatch_qualities[c] += mismatch_sum;
                    indel_stat->sum_of_mismatch_qualities += mismatch_sum;


                    if(q2_val > -1) {
                        //this is in read coordinates. Ignores clipping as q2 may be clipped
                        sum_q2_distance[c] += (float) abs(base->qpos - q2_val) / (float) base->b->core.l_qseq;
                        num_q2_reads[c]++;
                        indel_stat->sum_q2_distance += (float) abs(base->qpos - q2_val) / (float) base->b->core.l_qseq;
                        indel_stat->num_q2_reads++;
                    }
                    sum_3p_distance[c] += (float) abs(base->qpos - three_prime_index) / (float) base->b->core.l_qseq;
                    distances_to_3p[c][num_distances_to_3p[c]++] = (float) abs(base->qpos - three_prime_index) / (float) base->b->core.l_qseq;
                    indel_stat->sum_3p_distance += (float) abs(base->qpos - three_prime_index) / (float) base->b->core.l_qseq;

                    sum_of_clipped_lengths[c] += clipped_length;
                    indel_stat->sum_of_clipped_lengths += clipped_length;
                    //calculate distance from center of read as an absolute value
                    //float read_center = (float)base->b->core.l_qseq/2.0;
                    float read_center = (float)clipped_length/2.0;
                    sum_base_location[c] += 1.0 - abs((float)(base->qpos - left_clip) - read_center)/read_center;
                    indel_stat->sum_indel_location += 1.0 - abs((float)(base->qpos - left_clip) - read_center)/read_center;

                }
                else {
                    //fprintf(stderr, "Couldn't grab the generated tag for readname %s.\n",  bam1_qname(base->b));
                    WARN->warn(ReadWarnings::Zm_TAG_MISSING, bam1_qname(base->b));
                }



                //grab the single ended mapping qualities for testing
                if(base->b->core.flag & BAM_FPROPER_PAIR) {
                    uint8_t *sm_tag_ptr = bam_aux_get(base->b, "SM");
                    if(sm_tag_ptr) {
                        int32_t single_ended_map_qual = bam_aux2i(sm_tag_ptr);
                        sum_single_ended_map_qualities[c] += single_ended_map_qual;
                        indel_stat->sum_single_ended_map_qualities += single_ended_map_qual;
                    }
                    else {
                        WARN->warn(ReadWarnings::SM_TAG_MISSING, bam1_qname(base->b));
                        //fprintf(stderr,"Couldn't grab single-end mapping quality for read %s. Check to see if SM tag is in BAM\n",bam1_qname(base->b));
                    }
                }
                else {
                    //just add in the mapping quality as the single ended quality
                    sum_single_ended_map_qualities[c] += base->b->core.qual;
                    indel_stat->sum_single_ended_map_qualities += base->b->core.qual;
                }

                //grab out the number of mismatches
                uint8_t *nm_tag_ptr = bam_aux_get(base->b, "NM");
                if(nm_tag_ptr) {
                    int32_t number_mismatches = bam_aux2i(nm_tag_ptr);
                    sum_number_of_mismatches[c] += number_mismatches / (float) clipped_length;
                    indel_stat->sum_number_of_mismatches += number_mismatches / (float) clipped_length;
                }
                else {
                    //fprintf(stderr, "Couldn't grab number of mismatches for read %s. Check to see if NM tag is in BAM\n", bam1_qname(base->b));
                    WARN->warn(ReadWarnings::NM_TAG_MISSING, bam1_qname(base->b));
                }


                mapping_qualities[c][num_mapping_qualities[c]++] = base->b->core.qual;  //using post-increment here to alter stored number of mapping_qualities while using the previous number as the index to store. Tricky, sort of.

            }
        }

        //print out information on position and reference base and depth
        printf("%s\t%d\t%c\t%d", tmp->in->header->target_name[tid], pos + 1, (tmp->ref && (int)pos < tmp->len) ? tmp->ref[pos] : 'N', mapq_n );
        //print out the base information
        //Note that if there is 0 depth then that averages are reported as 0
        unsigned char j;
        for(j = 0; j < possible_calls; ++j) {
            unsigned int iter;
            if(tmp->distribution) {
                printf("\t%c:%d:", bam_canonical_nt_table[j], read_counts[j]);
                for(iter = 0; iter < num_mapping_qualities[j]; iter++) {
                    if(iter != 0) {
                        printf(",");
                    }
                    printf("%d",mapping_qualities[j][iter]);
                }
                printf(":");
                for(iter = 0; iter < num_distances_to_3p[j]; iter++) {
                    if(iter != 0) {
                        printf(",");
                    }
                    printf("%0.02f",distances_to_3p[j][iter]);
                }

            }
            else {
                printf("\t%c:%d:%0.02f:%0.02f:%0.02f:%d:%d:%0.02f:%0.02f:%0.02f:%d:%0.02f:%0.02f:%0.02f", bam_canonical_nt_table[j], read_counts[j], read_counts[j] ? (float)sum_map_qualities[j]/read_counts[j] : 0, read_counts[j] ? (float)sum_base_qualities[j]/read_counts[j] : 0, read_counts[j] ? (float)sum_single_ended_map_qualities[j]/read_counts[j] : 0, num_plus_strand[j], num_minus_strand[j], read_counts[j] ? sum_base_location[j]/read_counts[j] : 0, read_counts[j] ? (float) sum_number_of_mismatches[j]/read_counts[j] : 0, read_counts[j] ? (float) sum_of_mismatch_qualities[j]/read_counts[j] : 0, num_q2_reads[j], read_counts[j] ? (float) sum_q2_distance[j]/num_q2_reads[j] : 0, read_counts[j] ? (float) sum_of_clipped_lengths[j]/read_counts[j] : 0,read_counts[j] ? (float) sum_3p_distance[j]/read_counts[j] : 0);
            }
        }
        //here print out indels if they exist

        khiter_t iterator;
        for(iterator = kh_begin(hash); iterator != kh_end(hash); iterator++) {
            if(kh_exist(hash,iterator)) {
                //print it
                indel_stat_t *stats = &(kh_value(hash,iterator));
                printf("\t%s:%d:%0.02f:%0.02f:%0.02f:%d:%d:%0.02f:%0.02f:%0.02f:%d:%0.02f:%0.02f:%0.02f", kh_key(hash, iterator), stats->read_count, (float)stats->sum_map_qualities/stats->read_count, 0.0, (float)stats->sum_single_ended_map_qualities/stats->read_count, stats->num_plus_strand, stats->num_minus_strand, stats->sum_indel_location/stats->read_count, (float) stats->sum_number_of_mismatches/stats->read_count, (float) stats->sum_of_mismatch_qualities/stats->read_count, stats->num_q2_reads, (float) stats->sum_q2_distance/stats->num_q2_reads, (float) stats->sum_of_clipped_lengths/stats->read_count,(float) stats->sum_3p_distance/stats->read_count);
                free((char *) kh_key(hash,iterator));
                kh_del(indels,hash,iterator);
            }
        }


        printf("\n");

        kh_destroy(indels,hash);
        free(read_counts);
        free(sum_base_qualities);
        free(sum_map_qualities);
        free(sum_single_ended_map_qualities);
        //recycling i again
        for(i = 0; i < possible_calls; i++) {
            free(mapping_qualities[i]);
            free(distances_to_3p[i]);
        }
        free(distances_to_3p);
        free(mapping_qualities);
        free(num_mapping_qualities);
        free(num_distances_to_3p);

        free(num_plus_strand);
        free(num_minus_strand);
        free(sum_base_location);
        free(sum_number_of_mismatches);
        free(sum_of_mismatch_qualities);
        free(sum_q2_distance);
        free(num_q2_reads);
        free(sum_of_clipped_lengths);
        free(sum_3p_distance);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int c,distribution = 0;
    char *fn_fa = 0, *fn_pos = 0;
    int64_t max_warnings = -1;

    pileup_data_t *d = (pileup_data_t*)calloc(1, sizeof(pileup_data_t));
    fetch_data_t *f = (fetch_data_t*)calloc(1, sizeof(pileup_data_t));
    d->tid = -1, d->min_mapq = 0, d->min_bq = 0, d->max_cnt = 10000000;
    while ((c = getopt(argc, argv, "q:f:l:Db:w:d:")) >= 0) {
        switch (c) {
            case 'q': d->min_mapq = atoi(optarg); break;
            case 'b': d->min_bq = atoi(optarg); break;
            case 'd': d->max_cnt = atoi(optarg); break;
            case 'l': fn_pos = strdup(optarg); break;
            case 'f': fn_fa = strdup(optarg); break;
            case 'D': distribution = 1; break;
            case 'w': max_warnings = atoi(optarg); break;
            default: fprintf(stderr, "Unrecognized option '-%c'.\n", c); return 1;
        }
    }
    if (argc - optind == 0) {
        fprintf(stderr, "\n");
        fprintf(stderr, "Usage: bam-readcount <bam_file> [region]\n");
        fprintf(stderr, "        -q INT    filtering reads with mapping quality less than INT [%d]\n", d->min_mapq);
        fprintf(stderr, "        -b INT    don't include reads where the base quality is less than INT [%d]\n", d->min_bq);
        fprintf(stderr, "        -d INT    max depth to avoid excessive memory usage [%d]\n", d->max_cnt);
        fprintf(stderr, "        -f FILE   reference sequence in the FASTA format\n");
        fprintf(stderr, "        -l FILE   list of regions to report readcounts within.\n");
        fprintf(stderr, "        -D        report the mapping qualities as a comma separated list\n");
        fprintf(stderr, "        -w        maximum number of warnings of each type to emit [unlimited]\n\n");
        fprintf(stderr, "This program reports readcounts for each base at each position requested.\n");
        fprintf(stderr, "\nPositions should be requested via the -l option as chromosome, start, stop\nwhere the coordinates are 1-based and each field is separated by whitespace.\n");
        fprintf(stderr, "\nA single region may be requested on the command-line similarly to samtools view\n(i.e. bam-readcount -f ref.fa some.bam 1:150-150).\n\n");
        fprintf(stderr, "It also reports the average base quality of these bases and mapping qualities of\n");
        fprintf(stderr, "the reads containing each base.\n\nThe format is as follows:\nchr\tposition\treference_base\tbase:count:avg_mapping_quality:avg_basequality:avg_se_mapping_quality:num_plus_strand:num_minus_strand:avg_pos_as_fraction:avg_num_mismatches_as_fraction:avg_sum_mismatch_qualitiest:num_q2_containing_reads:avg_distance_to_q2_start_in_q2_reads:avg_clipped_length:avg_distance_to_effective_3p_end...\n");

        fprintf(stderr, "\n");
        return 1;
    }
    WARN.reset(new ReadWarnings(std::cerr, max_warnings));

    if (fn_fa) d->fai = fai_load(fn_fa);
    d->beg = 0; d->end = 0x7fffffff;
    d->distribution = distribution;
    d->in = samopen(argv[optind], "rb", 0);
    if (d->in == 0) {
        fprintf(stderr, "Fail to open BAM file %s\n", argv[optind]);
        return 1;
    }
    if(fn_pos) {
        std::ifstream fp(fn_pos);
        if(!fp.is_open()) {
            fprintf(stderr, "Failed to open region list file: %s\n", fn_pos);
            return 1;
        }
        bam_index_t *idx;
        idx = bam_index_load(argv[optind]); // load BAM index
        if (idx == 0) {
            fprintf(stderr, "BAM indexing file is not available.\n");
            return 1;
        }
        //Now iterate through and do calculations for each one
        std::string ref_name;
        int beg;
        int end;
        int ref;


        //initialize the header hash
        khiter_t iter;
        khash_t(s) *h;
        if (d->in->header->hash == 0) {
            int ret, i;
            khiter_t iter;
            khash_t(s) *h;
            d->in->header->hash = h = kh_init(s);
            for (i = 0; i < d->in->header->n_targets; ++i) {
                iter = kh_put(s, h, d->in->header->target_name[i], &ret);
                kh_value(h, iter) = i;
            }
        }
        h = (khash_t(s)*)d->in->header->hash;
        std::string lineBuf;
        while(getline(fp, lineBuf)) {
            std::stringstream ss(lineBuf);
            if (!(ss >> ref_name >> beg >> end))
                continue;

            iter = kh_get(s, h, ref_name.c_str());
            if(iter == kh_end(h)) {
                fprintf(stderr, "%s not found in bam file. Region %s %i %i skipped.\n",ref_name.c_str(),ref_name.c_str(),beg,end);
            }
            else {
                // ref id exists
                //fprintf(stderr, "%s %i %i scanned in\n",ref_name,beg,end);
                ref = kh_value(h,iter);
                //fprintf(stderr, "%i %i %i scanned in\n",ref,beg,end);
                d->beg = beg-1;
                d->end = end;
                if (d->fai && ref != d->tid) {
                    free(d->ref);
                    //would this be faster to just grab small chunks? Probably at some level, but not at others. How would chunking affect the indel allele calculations? Those assume that the indel allele is present in the ref and potentially occupy more than just the region of interest
                    d->ref = fai_fetch(d->fai, d->in->header->target_name[ref], &d->len);
                    d->tid = ref;
                }
                bam_plbuf_t *buf = bam_plbuf_init(pileup_func, d); // initialize pileup
                bam_plp_set_maxcnt(buf->iter, d->max_cnt);
                f->pileup_buffer = buf;
                if (d->fai) {
                    f->ref_len = d->len;
                    f->seq_name = d->in->header->target_name[d->tid];
                } else {
                    f->ref_len = 0;
                    f->seq_name = 0;
                }
                f->ref_pointer = &(d->ref);
                bam_fetch(d->in->x.bam, idx, ref, d->beg, d->end, f, fetch_func);
                bam_plbuf_push(0, buf); // finalize pileup
                bam_plbuf_destroy(buf);

            }
        }
        bam_index_destroy(idx);
        samclose(d->in);
        if(d->fai) {
            fai_destroy(d->fai);
        }
        if(d->ref) {
            free(d->ref);
        }
        free(d);
        free(f);
        free(fn_pos);
        free(fn_fa);
        return 0;
    }
    else {
        if (argc - optind == 1) { // if a region is not specified
            //FIXME this currently crashes and burns because it doesn't hit the pre-processing in fetch_func
            sampileup(d->in, -1, pileup_func, d);
        } else {
            int ref;
            bam_index_t *idx;
            bam_plbuf_t *buf;
            idx = bam_index_load(argv[optind]); // load BAM index
            if (idx == 0) {
                fprintf(stderr, "BAM indexing file is not available.\n");
                return 1;
            }
            bam_parse_region(d->in->header, argv[optind + 1], &ref, &(d->beg), &(d->end)); // parse the region
            if (ref < 0) {
                fprintf(stderr, "Invalid region %s\n", argv[optind + 1]);
                return 1;
            }
            if (d->fai && ref != d->tid) {
                free(d->ref);
                d->ref = fai_fetch(d->fai, d->in->header->target_name[ref], &d->len);
                d->tid = ref;
            }
            buf = bam_plbuf_init(pileup_func, d); // initialize pileup
            bam_plp_set_maxcnt(buf->iter, d->max_cnt);
            f->pileup_buffer = buf;
            f->ref_pointer = &(d->ref);
            bam_fetch(d->in->x.bam, idx, ref, d->beg, d->end, f, fetch_func);
            bam_plbuf_push(0, buf); // finalize pileup
            bam_index_destroy(idx);
            bam_plbuf_destroy(buf);
        }
        if(d->ref) {
            free(d->ref);
        }
        if(d->fai) {
            fai_destroy(d->fai);
        }
        if(fn_fa) {
            free(fn_fa);
        }
        free(f);
    }
    samclose(d->in);
    return 0;
}
