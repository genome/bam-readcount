#include "BamEntry.hpp"
#include "BamFilter.hpp"
#include "BamReader.hpp"
#include "BasicStat.hpp"
#include "FileRegionSequence.hpp"
#include "Options.hpp"
#include "ReadProcessor.hpp"
#include "ReadWarnings.hpp"
#include "StaticRegionSequence.hpp"
#include "auxfields.hpp"

#include <sam.h>
#include <faidx.h>
#include <sam_header.h>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include <unistd.h>

#include <unordered_set>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>


using namespace std;
using boost::format;
namespace po = boost::program_options;

/* This will convert all iub codes in the reads to N */
char const* bam_canonical_nt_table = "=ACGTN";
unsigned char possible_calls = (unsigned char) strlen(bam_canonical_nt_table);
unsigned char bam_nt16_canonical_table[16] = { 0,1,2,5,
    3,5,5,5,
    4,5,5,5,
    5,5,5,5};

struct LibraryCounts {
    std::map<std::string, BasicStat> indel_stats;
    std::vector<BasicStat> base_stats;
    LibraryCounts() : indel_stats(), base_stats(possible_calls) {}
};

struct IndelQueueEntry {
    uint32_t tid;
    uint32_t pos;
    BasicStat indel_stats;
    std::string allele;
    IndelQueueEntry() : tid(0), pos(0), indel_stats(), allele() {}
};

typedef std::queue<IndelQueueEntry> indel_queue_t;
typedef std::map<std::string, indel_queue_t> indel_queue_map_t;

//Struct to store info to be passed around
typedef struct {
    faidx_t *fai;       //index into fasta file
    int tid;            //reference id
    char *ref;          //reference sequence
    int min_bq;       //minimum mapping qualitiy to use
    int beg,end;        //start and stop of region
    int len;            //length of currently loaded reference sequence
    int max_cnt;       //maximum depth to set on the pileup buffer
    int distribution;   //whether or not to display all mapping qualities
    bool per_lib;
    bool insertion_centric;
    bam_header_t* header;
    std::set<std::string> lib_names;
    indel_queue_map_t indel_queue_map;
} pileup_data_t;

//struct to store reference for passing to fetch func
typedef struct {
    const char* seq_name;
    int ref_len;
    char **ref_pointer;
    bam_plbuf_t* pileup_buffer;
} fetch_data_t;

std::auto_ptr<ReadWarnings> WARN;

static inline void load_reference(pileup_data_t* data, int ref) {
    if (data->fai && ref != data->tid) {
        free(data->ref);
        //would this be faster to just grab small chunks? Probably at some level, but not at others. How would chunking affect the indel allele calculations? Those assume that the indel allele is present in the ref and potentially occupy more than just the region of interest
        data->ref = fai_fetch(data->fai, data->header->target_name[ref], &data->len);
        data->tid = ref;
    }
}

std::set<std::string> find_library_names(bam_header_t const* header) {
    //samtools doesn't do a good job of exposing this so this is a little more implementation
    //dependent than I'd like and may be fragile.
    std::set<std::string> lib_names;
    void *iter = header->dict;
    const char *key, *val;
    while( (iter = sam_header2key_val(iter, "RG", "ID", "LB", &key, &val)) ) {
        lib_names.insert(val);
    }
    return lib_names;
}

// callback for bam_plbuf_init()
// TODO allow for a simplified version that calculates less
static int pileup_func(uint32_t tid, uint32_t pos, int n, const bam_pileup1_t *pl, void *data) {
    pileup_data_t *tmp = (pileup_data_t*)data;
    load_reference(tmp, tid);

    if ((int)pos >= tmp->beg - 1 && (int)pos < tmp->end) {

        int mapq_n = 0; //this tracks the number of reads that passed the mapping quality threshold

        std::map<std::string, LibraryCounts> lib_counts;

        //loop over the bases, recycling i here.
        for(int i = 0; i < n; ++i) {
            const bam_pileup1_t *base = pl + i; //get base index
            const char* library_name = "all";
            if(tmp->per_lib) {
                library_name = bam_get_library(tmp->header, base->b);
                if(library_name == 0) {
                    WARN->warn(ReadWarnings::LIBRARY_UNAVAILABLE, bam1_qname(base->b));
                    return 0;
                }
            }
            LibraryCounts &current_lib = lib_counts[library_name];

            if(!base->is_del && bam1_qual(base->b)[base->qpos] >= tmp->min_bq) {
                mapq_n++;


                if(base->indel != 0 && tmp->ref) {
                    //indel containing read exists here
                    //need to: 1) add an indel counting mode so insertions aren't double counted or add separate "non-indel" tracking.
                    //2) create a queue of indel counts and the positions where they /should/ be reported. These positions need to determine reporting as well. ie. if reporting on a deletion and the roi doesn't include the deletion, but we find it. then we still need to do the pileup for that position BUT, there are no guarantees that we know that until we see the data.
                    //3) So deletions should get put into a queue and their emission position stored.
                    //4) At each new position, check if we need to emit the indel.
                    //5) if that position is a) in our target roi and already passed or b) the current position then we need to emit and this needs to be done in a loop until no more indels are candidates. Maybe two loops.
                    //
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
                if(base->indel < 1 || !tmp->insertion_centric) {
                    unsigned char c = bam_nt16_canonical_table[bam1_seqi(bam1_seq(base->b), base->qpos)];   //convert to index
                    (current_lib.base_stats)[c].process_read(base);
                }
            }
        }

        //print out information on position and reference base and depth
        std::string ref_name(tmp->header->target_name[tid]);
        std::string ref_base;
        ref_base += (tmp->ref && (int)pos < tmp->len) ? tmp->ref[pos] : 'N';
        stringstream record;
        int extra_depth = 0;
        //print out the base information
        //Note that if there is 0 depth then that averages are reported as 0

        std::map<std::string, LibraryCounts>::iterator lib_iter;
        for(lib_iter = lib_counts.begin(); lib_iter != lib_counts.end(); lib_iter++) {
            //print it
            if(tmp->per_lib) {
                record << "\t" << lib_iter->first << "\t{";
            }
            for(unsigned char j = 0; j < possible_calls; ++j) {
                if(tmp->distribution) {
                    throw std::runtime_error(
                        "Not currently supporting distributions"
                        );
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
                    record << "\t" << bam_canonical_nt_table[j] << ":" << lib_iter->second.base_stats[j];
                }
            }
            std::map<std::string, BasicStat>::iterator it;
            for(it = lib_iter->second.indel_stats.begin(); it != lib_iter->second.indel_stats.end(); ++it) {
                if(it->first[0] == '-') {
                    //it's a deletion
                    IndelQueueEntry new_entry;
                    new_entry.tid = tid;
                    new_entry.pos = pos + 1;
                    new_entry.indel_stats = it->second;
                    new_entry.allele = it->first;
                    indel_queue_t &test = tmp->indel_queue_map[lib_iter->first];
                    test.push(new_entry);
                }
                else {
                    //it's an insertion and it should be output at this position
                    record << "\t" << it->first << ":" << it->second;
                }
            }

            std::map<std::string, indel_queue_t>::iterator queued_it;
            queued_it = tmp->indel_queue_map.find(lib_iter->first);
            if(queued_it != tmp->indel_queue_map.end()) {
                //we have an indel queue for this library
                indel_queue_t &current_lib_queue = queued_it->second;
                while(!current_lib_queue.empty() && current_lib_queue.front().tid == tid && current_lib_queue.front().pos < pos) {
                    current_lib_queue.pop();
                }

                while(!current_lib_queue.empty() && current_lib_queue.front().tid == tid && current_lib_queue.front().pos == pos) {
                    record << "\t" << current_lib_queue.front().allele << ":" << current_lib_queue.front().indel_stats;
                    extra_depth += current_lib_queue.front().indel_stats.read_count;
                    current_lib_queue.pop();
                }
            }
            if(tmp->per_lib) {
                record << "\t}";
            }
        }
        if ((int)pos >= tmp->beg && (int)pos < tmp->end) {
            cout << ref_name << "\t" << pos + 1 << "\t" << ref_base << "\t" << mapq_n + extra_depth << record.str() << endl;
        }
    }
    return 0;
}

static int bam_readcount(Options const& opts) {
    BamEntry entry;
    pileup_data_t d;
    fetch_data_t f;
    memset(&d, 0, sizeof(d));
    memset(&f, 0, sizeof(f));

    d.tid = -1, d.min_bq = 0, d.max_cnt = 10000000;

    // FIXME: put options in "d" instead of copying here.
    d.min_bq = opts.min_baseq;
    d.max_cnt = opts.max_depth;
    d.distribution = opts.distribution;
    d.per_lib = opts.per_lib;
    d.insertion_centric = opts.insertion_centric;


    cerr << "Minimum mapping quality is set to " << opts.min_mapq << endl;
    WARN.reset(new ReadWarnings(std::cerr, opts.max_warnings));

    if (!opts.fasta_file.empty()) {
        d.fai = fai_load(opts.fasta_file.c_str());
        if (d.fai == 0) {
            throw std::runtime_error(str(format(
                "Failed to load fasta file %1%"
                ) % opts.fasta_file));
        }
    }

    BamFilter filter(opts.min_mapq, opts.required_flags, opts.forbidden_flags);
    BamReader reader(opts.input_file);
    reader.set_filter(&filter);
    d.header = reader.header();

    d.beg = 0; d.end = 0x7fffffff;

    std::set<std::string> lib_names = find_library_names(reader.header());
    for(std::set<std::string>::iterator it = lib_names.begin(); it != lib_names.end(); ++it) {
        cerr << "Expect library: " << *it << " in BAM" << endl;
    }
    d.lib_names = lib_names;
    d.indel_queue_map = indel_queue_map_t();

    auto const& seq_names = reader.sequence_names();

    std::unique_ptr<IRegionSequence> regions;
    if (!opts.pos_file.empty()) {
        regions.reset(new FileRegionSequence(opts.pos_file));
    }
    else {
        auto const& regs = opts.regions.empty() ? seq_names : opts.regions;
        regions.reset(new StaticRegionSequence(regs));
    }

    int ref;
    std::string reg;
    while (regions->next(reg)) {
        if (bam_parse_region(reader.header(), reg.c_str(), &ref, &d.beg, &d.end) != 0) {
            std::cerr << str(format("Failed to parse region '%1%', skipping."
                ) % reg);
            continue;
        }

        reader.set_region(ref, d.beg, d.end);
        load_reference(&d, ref);
        bam_plbuf_t *buf = bam_plbuf_init(pileup_func, &d); // initialize pileup
        bam_plp_set_maxcnt(buf->iter, d.max_cnt);
        f.pileup_buffer = buf;

        if (d.fai) {
            f.ref_len = d.len;
            f.seq_name = seq_names[d.tid].c_str();
        }

        f.ref_pointer = &(d.ref);

        ReadProcessor readproc(
              seq_names[d.tid].c_str()
            , d.len
            , &(d.ref)
            , buf
            );

        while (reader.next(entry)) {
            readproc.push_read(entry);
        }

        readproc.flush();
    }

    if(d.fai)
        fai_destroy(d.fai);
    if(d.ref)
        free(d.ref);

    return 0;
}

int main(int argc, char *argv[]) {
    try {
        Options opts(argc, argv);
        opts.validate();
        return bam_readcount(opts);
    }
    catch (CmdlineHelpException const& e) {
        std::cout << e.what() << "\n";
    }
    catch (std::exception const& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
