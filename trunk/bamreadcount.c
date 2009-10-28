#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include "sam.h"
#include "faidx.h"
#include "khash.h"
#include <stdio.h>

typedef char *str_p;
KHASH_MAP_INIT_STR(s, int)
KHASH_MAP_INIT_STR(r2l, str_p)


//Struct to store info to be passed around    
typedef struct {
    faidx_t *fai;       //index into fasta file
    int tid;            //reference id 
    char *ref;          //reference sequence
    int min_mapq;       //minimum mapping qualitiy to use
    int beg,end;        //start and stop of region
    int len;            //length of currently loaded reference sequence
    samfile_t *in;      //bam file 
    int distribution;   //whether or not to display all mapping qualities
} pileup_data_t;

// callback for bam_fetch()
static int fetch_func(const bam1_t *b, void *data)
{
    //This just pushes all reads into the pileup buffer
	bam_plbuf_t *buf = (bam_plbuf_t*)data;
	bam_plbuf_push(b, buf);
	return 0;
}

/* This will convert all iub codes in the reads to N */
char *bam_canonical_nt_table = "=ACGTN";
unsigned char bam_nt16_canonical_table[16] = { 0,1,2,5,
                                                3,5,5,5,
                                                4,5,5,5,
                                                5,5,5,5};

// callback for bam_plbuf_init()
static int pileup_func(uint32_t tid, uint32_t pos, int n, const bam_pileup1_t *pl, void *data)
{
    pileup_data_t *tmp = (pileup_data_t*)data;

    if ((int)pos >= tmp->beg && (int)pos < tmp->end) {

        //set up data structures to count bases
        unsigned char possible_calls = (unsigned char) strlen(bam_canonical_nt_table);
        unsigned int *read_counts = calloc(possible_calls,sizeof(unsigned int));
        unsigned int *sum_base_qualities = calloc(possible_calls,sizeof(unsigned int));  
        unsigned int *sum_map_qualities = calloc(possible_calls,sizeof(unsigned int));
        unsigned int **mapping_qualities = calloc(possible_calls, sizeof(unsigned int*));
        unsigned int *num_mapping_qualities = calloc(possible_calls, sizeof(unsigned int));

        int mapq_n = 0; //this tracks the number of reads that passed the mapping quality threshold
        
        //allocate enough mem to store relevant mapping qualities
        int i;
        for(i = 0; i < possible_calls; i++) {
            mapping_qualities[i] = calloc(n, sizeof(unsigned int));
        }
        
        //loop over the bases, recycling i here. 
        for(i = 0; i < n; ++i) {
            const bam_pileup1_t *base = pl + i; //get base index
            if(!base->is_del && base->b->core.qual >= tmp->min_mapq) {
                mapq_n++;
                int c = (int) bam_nt16_canonical_table[bam1_seqi(bam1_seq(base->b), base->qpos)];   //convert to index
                read_counts[c] ++; //calloc should 0 out the mem
                sum_base_qualities[c] += bam1_qual(base->b)[base->qpos];
                sum_map_qualities[c] += base->b->core.qual; 
                mapping_qualities[c][num_mapping_qualities[c]++] = base->b->core.qual;  //using post-increment here to alter stored number of mapping_qualities while using the previous number as the index to store. Tricky, sort of.
                
            }
        }
        
        //print out information on position and reference base and depth
        printf("%s\t%d\t%c\t%d", tmp->in->header->target_name[tid], pos + 1, (tmp->ref && (int)pos < tmp->len) ? tmp->ref[pos] : 'N', mapq_n );
        //print out the base information
        //Note that if there is 0 depth then that averages are reported as 0
        unsigned char j;
        for(j = 0; j < possible_calls; ++j) { 
            if(tmp->distribution) {
                printf("\t%c:%d:", bam_canonical_nt_table[j], read_counts[j]);
                for(i = 0; i < num_mapping_qualities[j]; i++) {
                    if(i != 0) {
                        printf(",");
                    }
                    printf("%d",mapping_qualities[j][i]);
                }
            }
            else {
                printf("\t%c:%d:%0.02f:%0.02f", bam_canonical_nt_table[j], read_counts[j], read_counts[j] ? (float)sum_map_qualities[j]/read_counts[j] : 0, read_counts[j] ? (float)sum_base_qualities[j]/read_counts[j] : 0);
            }
        }
        printf("\n");
        free(read_counts);
        free(sum_base_qualities);
        free(sum_map_qualities);
        //recycling i again
        for(i = 0; i < possible_calls; i++) {
            free(mapping_qualities[i]);
        }
        free(mapping_qualities);
        free(num_mapping_qualities);
    }
    
	return 0;
}

int main(int argc, char *argv[])
{
    int c,distribution = 0;
	char *fn_fa = 0, *fn_pos = 0;
	pileup_data_t *d = (pileup_data_t*)calloc(1, sizeof(pileup_data_t));
	d->tid = -1, d->min_mapq = 0;
	while ((c = getopt(argc, argv, "q:f:l:d")) >= 0) {
		switch (c) {
		case 'q': d->min_mapq = atoi(optarg); break;
		case 'l': fn_pos = strdup(optarg); break;
		case 'f': fn_fa = strdup(optarg); break;
        case 'd': distribution = 1; break;          
		default: fprintf(stderr, "Unrecognizd option '-%c'.\n", c); return 1;
		}
	}
	if (argc - optind == 0) {
        fprintf(stderr, "\n");
        fprintf(stderr, "Usage: bam-readcount <bam_file> [region]\n");
        fprintf(stderr, "        -q INT    filtering reads with mapping quality less than INT [%d]\n", d->min_mapq);
        fprintf(stderr, "        -f FILE   reference sequence in the FASTA format\n");
        fprintf(stderr, "        -l FILE   list of regions to report readcounts within\n");
        fprintf(stderr, "        -d        report the mapping qualities as a comma separated list\n\n");
        fprintf(stderr, "This program reports readcounts for each base at each position requested.\n");
        fprintf(stderr, "It also reports the average base quality of these bases and mapping quality of\n");
        fprintf(stderr, "the reads containing each base.\n\nThe format is as follows:\nchr\tposition\treference_base\tbase:count:avg_basequality:avg_mapping_quality\t...\n");
                
        fprintf(stderr, "\n");
		return 1;
	}
	if (fn_fa) d->fai = fai_load(fn_fa);
	d->beg = 0; d->end = 0x7fffffff;
    d->distribution = distribution;
	d->in = samopen(argv[optind], "rb", 0);
	if (d->in == 0) {
		fprintf(stderr, "Fail to open BAM file %s\n", argv[optind]);
		return 1;
	}
    if(fn_pos) {
       FILE* fp = fopen(fn_pos,"r");
       if(fn_pos == NULL) {
          fprintf(stderr, "Failed to open region list\n");
          return 1;
       }
       bam_index_t *idx;
       idx = bam_index_load(argv[optind]); // load BAM index
       if (idx == 0) {
           fprintf(stderr, "BAM indexing file is not available.\n");
           return 1;
       }
       //Now iterate through and do calculations for each one
       char *line = NULL; 
       char *ref_name; 
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
       ssize_t read;
       size_t length;
       while((read = getline(&line,&length,fp)) != -1) {
           if(sscanf(line,"%as%i%i",&ref_name, &beg, &end)==3) {
               iter = kh_get(s, h, ref_name);
              if(iter == kh_end(h)) {
                  fprintf(stderr, "%s not found in bam file. Region %s %i %i skipped.\n",ref_name,ref_name,beg,end);
                  free(ref_name);
              }
              else {
                  // ref id exists
                  //fprintf(stderr, "%s %i %i scanned in\n",ref_name,beg,end);
                  ref = kh_value(h,iter);  
                  //fprintf(stderr, "%i %i %i scanned in\n",ref,beg,end);
                  free(ref_name);
                  d->beg = beg-1;
                  d->end = end;
                  if (d->fai && ref != d->tid) {
                      free(d->ref);
                      d->ref = fai_fetch(d->fai, d->in->header->target_name[ref], &d->len);
                      d->tid = ref;
                  }
                  bam_plbuf_t *buf = bam_plbuf_init(pileup_func, d); // initialize pileup
                  bam_fetch(d->in->x.bam, idx, ref, d->beg, d->end, buf, fetch_func);
                  bam_plbuf_push(0, buf); // finalize pileup
                  bam_plbuf_destroy(buf);
                  
              }
          }
          else {
            free(ref_name);
          }
       }
       bam_index_destroy(idx);
       if(line) {
           free(line);
       }
       samclose(d->in);
       fclose(fp);
       free(d);
       free(fn_pos);
       free(fn_fa);
       return 0;
    }
    else {
        if (argc - optind == 1) { // if a region is not specified
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
            bam_fetch(d->in->x.bam, idx, ref, d->beg, d->end, buf, fetch_func);
            bam_plbuf_push(0, buf); // finalize pileup
            bam_index_destroy(idx);
            bam_plbuf_destroy(buf);
        }
    }
	samclose(d->in);
	return 0;
}
