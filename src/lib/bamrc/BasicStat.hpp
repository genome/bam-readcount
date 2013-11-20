#pragma once

#include <ostream>
#include <vector>
#include "sam.h"

class BasicStat {
    public:
        BasicStat(bool is_indel = false);
        void process_read(bam_pileup1_t const*  base); //may want other things here like clipping.

        mutable unsigned int read_count;    //number of reads containing the indel
        mutable unsigned int sum_map_qualities; //sum of the mapping qualities of reads containing the indel
        mutable unsigned int sum_single_ended_map_qualities; //sum of the single ended mapping qualities;
        mutable unsigned int num_plus_strand;
        mutable unsigned int num_minus_strand;
        mutable float sum_event_location;
        mutable float sum_q2_distance;
        mutable unsigned int num_q2_reads;
        mutable float sum_number_of_mismatches;
        mutable unsigned int sum_of_mismatch_qualities;
        mutable unsigned int sum_of_clipped_lengths;
        mutable float sum_3p_distance;
        mutable unsigned int sum_base_qualities;
        mutable std::vector<unsigned int> mapping_qualities;
        mutable std::vector<float> distances_to_3p;
        bool is_indel;
};

std::ostream& operator<<(std::ostream& s, const BasicStat& stat);
