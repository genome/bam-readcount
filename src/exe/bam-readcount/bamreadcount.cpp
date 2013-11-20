#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include "bamrc/auxfields.hpp"
#include "bamrc/ReadWarnings.hpp"
#include <boost/program_options.hpp>
#include "bamrc/BasicStat.hpp"

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
#include <cmath>
#include <map>


using namespace std;
namespace po = boost::program_options;

/* This will convert all iub codes in the reads to N */
char const* bam_canonical_nt_table = "=ACGTN";
unsigned char possible_calls = (unsigned char) strlen(bam_canonical_nt_table);
unsigned char bam_nt16_canonical_table[16] = { 0,1,2,5,
    3,5,5,5,
    4,5,5,5,
    5,5,5,5};

//below is for sam header
KHASH_MAP_INIT_STR(s, int)

struct LibraryCounts {
    std::map<std::string, BasicStat> indel_stats;
    std::vector<BasicStat> base_stats;
    LibraryCounts() : indel_stats(), base_stats(possible_calls) {}
};


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
    bool per_lib;
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

// callback for bam_plbuf_init()
// TODO allow for a simplified version that calculates less
static int pileup_func(uint32_t tid, uint32_t pos, int n, const bam_pileup1_t *pl, void *data) {
    pileup_data_t *tmp = (pileup_data_t*)data;

    if ((int)pos >= tmp->beg && (int)pos < tmp->end) {

        int mapq_n = 0; //this tracks the number of reads that passed the mapping quality threshold

        std::map<std::string, LibraryCounts> lib_counts;

        //loop over the bases, recycling i here.
        for(int i = 0; i < n; ++i) {
            const bam_pileup1_t *base = pl + i; //get base index
            const char* library_name = "all";
            if(tmp->per_lib) {
                library_name = bam_get_library(tmp->in->header, base->b);
            }
            LibraryCounts &current_lib = lib_counts[library_name];

            if(!base->is_del && base->b->core.qual >= tmp->min_mapq && bam1_qual(base->b)[base->qpos] >= tmp->min_bq) {
                mapq_n++;

                if(base->indel != 0 && tmp->ref) {
                    //indel containing read exists here
                    //will need to
                    std::string allele;
                    if(base->indel > 0) {
                        allele += "+";
                        for(int indel_base = 0; indel_base < base->indel; indel_base++) {
                            //scan indel allele off the read
                            allele += bam_canonical_nt_table[bam_nt16_canonical_table[bam1_seqi(bam1_seq(base->b), base->qpos + 1 + indel_base)]];
                        }
                    }
                    else {
                        //deletion
                        allele += "-";
                        for(int indel_base = 0; indel_base < abs(base->indel); indel_base++) {
                            //scan indel allele off the reference
                            //FIXME this will break with no reference
                            allele += tmp->ref[pos + indel_base + 1];
                        }
                    }
                    current_lib.indel_stats[allele].is_indel=true;
                    current_lib.indel_stats[allele].process_read(base);
                }
                else {
                    //the following are done regardless of whether or not there is an indel
                    unsigned char c = bam_nt16_canonical_table[bam1_seqi(bam1_seq(base->b), base->qpos)];   //convert to index
                    (current_lib.base_stats)[c].process_read(base);
                }
            }
        }

        //print out information on position and reference base and depth
        std::string ref_name(tmp->in->header->target_name[tid]);
        std::string ref_base;
        ref_base += (tmp->ref && (int)pos < tmp->len) ? tmp->ref[pos] : 'N';
        cout << ref_name << "\t" << pos + 1 << "\t" << ref_base << "\t" << mapq_n;
        //print out the base information
        //Note that if there is 0 depth then that averages are reported as 0

        std::map<std::string, LibraryCounts>::iterator lib_iter;
        for(lib_iter = lib_counts.begin(); lib_iter != lib_counts.end(); lib_iter++) {
            //print it
            if(tmp->per_lib) {
                cout << "\t" << lib_iter->first << "\t";
            }
            for(unsigned char j = 0; j < possible_calls; ++j) {
                if(tmp->distribution) {
                    throw "Not currently supporting distributions\n";
                    /*
                       printf("\t%c:%d:", bam_canonical_nt_table[j], base_stat->read_counts[j]);
                       for(iter = 0; iter < base_stat->num_mapping_qualities[j]; iter++) {
                       if(iter != 0) {
                       printf(",");
                       }
                       printf("%d",base_stat->mapping_qualities[j][iter]);
                       }
                       printf(":");
                       for(iter = 0; iter < base_stat->num_distances_to_3p[j]; iter++) {
                       if(iter != 0) {
                       printf(",");
                       }
                       printf("%0.02f",base_stat->distances_to_3p[j][iter]);
                       }
                       */
                }
                else {
                    cout << "\t" << bam_canonical_nt_table[j] << ":" << lib_iter->second.base_stats[j];
                    //printf("\t%c:%d:%0.02f:%0.02f:%0.02f:%d:%d:%0.02f:%0.02f:%0.02f:%d:%0.02f:%0.02f:%0.02f", bam_canonical_nt_table[j], base_stat->read_counts[j], base_stat->read_counts[j] ? (float)base_stat->sum_map_qualities[j]/base_stat->read_counts[j] : 0, base_stat->read_counts[j] ? (float)base_stat->sum_base_qualities[j]/base_stat->read_counts[j] : 0, base_stat->read_counts[j] ? (float)base_stat->sum_single_ended_map_qualities[j]/base_stat->read_counts[j] : 0, base_stat->num_plus_strand[j], base_stat->num_minus_strand[j], base_stat->read_counts[j] ? base_stat->sum_base_location[j]/base_stat->read_counts[j] : 0, base_stat->read_counts[j] ? (float) base_stat->sum_number_of_mismatches[j]/base_stat->read_counts[j] : 0, base_stat->read_counts[j] ? (float) base_stat->sum_of_mismatch_qualities[j]/base_stat->read_counts[j] : 0, base_stat->num_q2_reads[j], base_stat->read_counts[j] ? (float) base_stat->sum_q2_distance[j]/base_stat->num_q2_reads[j] : 0, base_stat->read_counts[j] ? (float) base_stat->sum_of_clipped_lengths[j]/base_stat->read_counts[j] : 0,base_stat->read_counts[j] ? (float) base_stat->sum_3p_distance[j]/base_stat->read_counts[j] : 0);
                }
            }
            std::map<std::string, BasicStat>::iterator it;
            for(it = lib_iter->second.indel_stats.begin(); it != lib_iter->second.indel_stats.end(); ++it) {
                cout << "\t" << it->first << ":" << it->second;
            }

            if(tmp->per_lib) {
                cout << "\t}";
            }
        }
        cout << endl;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    bool distribution = false;
    bool per_lib = false;
    string fn_pos, fn_fa;
    int64_t max_warnings = -1;

    pileup_data_t *d = (pileup_data_t*)calloc(1, sizeof(pileup_data_t));
    fetch_data_t *f = (fetch_data_t*)calloc(1, sizeof(pileup_data_t));
    d->tid = -1, d->min_bq = 0, d->max_cnt = 10000000;

    po::options_description desc("Available options");
    desc.add_options()
        ("help,h", "produce this message")
        ("min-mapping-quality,q", po::value<int>(&d->min_mapq)->default_value(0), "minimum mapping quality of reads used for counting.")
        ("min-base-quality,b", po::value<int>(&d->min_bq)->default_value(0), "minimum base quality at a position to use the read for counting.")
        ("max-count,d", po::value<int>(&d->max_cnt)->default_value(10000000), "max depth to avoid excessive memory usage.")
        ("site-list,l", po::value<string>(&fn_pos), "file containing a list of regions to report readcounts within.") 
        ("reference-fasta,f", po::value<string>(&fn_fa), "reference sequence in the fasta format.") 
        ("print-individual-mapq,D", po::value<bool>(&distribution), "report the mapping qualities as a comma separated list.")
        ("per-library,p", po::bool_switch(&per_lib), "report results by library.")
        ("max-warnings,w", po::value<int64_t>(&max_warnings), "maximum number of warnings of each type to emit. -1 gives an unlimited number.")
        ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("bam-file", po::value<string>(), "bam file")
        ("region", po::value<string>(), "region specification e.g. 2:1-50")
        ;

    po::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    po::positional_options_description p;
    p.add("bam-file", 1);
    p.add("region", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(cmdline_options).positional(p).run(), vm);
    po::notify(vm);
    if (vm.count("help") || vm.count("bam-file") == 0) {
        cout << desc << "\n";
        return 1;
    }
    cerr << "Minimum mapping quality is set to " << d->min_mapq << endl;
    /*
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
*/
    WARN.reset(new ReadWarnings(std::cerr, max_warnings));

    if (!fn_fa.empty()) d->fai = fai_load(fn_fa.c_str());
    d->beg = 0; d->end = 0x7fffffff;
    d->distribution = distribution;
    d->per_lib = per_lib;
    d->in = samopen(vm["bam-file"].as<string>().c_str(), "rb", 0);
    if (d->in == 0) {
        fprintf(stderr, "Fail to open BAM file %s\n", argv[optind]);
        return 1;
    }
    if(!fn_pos.empty()) {
        std::ifstream fp(fn_pos.c_str());
        if(!fp.is_open()) {
            cerr << "Failed to open region list file: " << fn_pos << endl;
            return 1;
        }
        bam_index_t *idx;
        idx = bam_index_load(vm["bam-file"].as<string>().c_str()); // load BAM index
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
        //free(fn_pos);
        //free(fn_fa);
        return 0;
    }
    else {
        if (!vm.count("region")) { // if a region is not specified
            //FIXME this currently crashes and burns because it doesn't hit the pre-processing in fetch_func
            sampileup(d->in, -1, pileup_func, d);
        } else {
            int ref;
            bam_index_t *idx;
            bam_plbuf_t *buf;
            idx = bam_index_load(vm["bam-file"].as<string>().c_str()); // load BAM index
            if (idx == 0) {
                fprintf(stderr, "BAM indexing file is not available.\n");
                return 1;
            }
            bam_parse_region(d->in->header, vm["region"].as<string>().c_str(), &ref, &(d->beg), &(d->end)); // parse the region
            if (ref < 0) {
                fprintf(stderr, "Invalid region %s\n", vm["region"].as<string>().c_str());
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
        free(f);
    }
    samclose(d->in);
    return 0;
}
