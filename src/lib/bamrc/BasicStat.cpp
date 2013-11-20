#include "BasicStat.hpp"
#include "bamrc/auxfields.hpp"
#include "bamrc/ReadWarnings.hpp"

#include <iomanip>
#include <iostream>

BasicStat::BasicStat()
    : read_count(0)
    , sum_map_qualities(0)
    , sum_single_ended_map_qualities(0)
    , num_plus_strand(0)
    , num_minus_strand(0)
    , sum_event_location(0.0)
    , sum_q2_distance(0.0)
    , num_q2_reads(0)
    , sum_number_of_mismatches(0.0)
    , sum_of_mismatch_qualities(0)
    , sum_of_clipped_lengths(0)
    , sum_3p_distance(0.0)
{
}

void BasicStat::process_read(bam_pileup1_t const& base) {
    read_count++;
    sum_map_qualities += base->b->core.qual;
    if(base->b->core.flag & BAM_FREVERSE) {
        //mapped to the reverse strand
        num_minus_strand++;
    }
    else {
        //must be mapped to the plus strand
        num_plus_strand++;
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
        sum_of_mismatch_qualities += mismatch_sum;

        if(q2_val > -1) {
            //this is in read coordinates. Ignores clipping as q2 may be clipped
            sum_q2_distance += (float) abs(base->qpos - q2_val) / (float) base->b->core.l_qseq;
            num_q2_reads++;
        }
        distances_to_3p.push_back( (float) abs(base->qpos - three_prime_index) / (float) base->b->core.l_qseq);
        sum_3p_distance += (float) abs(base->qpos - three_prime_index) / (float) base->b->core.l_qseq;

        sum_of_clipped_lengths += clipped_length;
        float read_center = (float)clipped_length/2.0;
        sum_indel_location += 1.0 - abs((float)(base->qpos - left_clip) - read_center)/read_center;

    }
    else {
        WARN->warn(ReadWarnings::Zm_TAG_MISSING, bam1_qname(base->b));
    }

    //grab the single ended mapping qualities for testing
    if(base->b->core.flag & BAM_FPROPER_PAIR) {
        uint8_t *sm_tag_ptr = bam_aux_get(base->b, "SM");
        if(sm_tag_ptr) {
            int32_t single_ended_map_qual = bam_aux2i(sm_tag_ptr);
            sum_single_ended_map_qualities += single_ended_map_qual;
        }
        else {
            WARN->warn(ReadWarnings::SM_TAG_MISSING, bam1_qname(base->b));
        }
    }
    else {
        //just add in the mapping quality as the single ended quality
        sum_single_ended_map_qualities += base->b->core.qual;
    }

    //grab out the number of mismatches
    uint8_t *nm_tag_ptr = bam_aux_get(base->b, "NM");
    if(nm_tag_ptr) {
        int32_t number_mismatches = bam_aux2i(nm_tag_ptr);
        sum_number_of_mismatches += number_mismatches / (float) clipped_length;
    }
    else {
        WARN->warn(ReadWarnings::NM_TAG_MISSING, bam1_qname(base->b));
    }
    mapping_qualities.push_back(base->b->core.qual);

}

std::ostream& operator<<(std::ostream& s, const BasicStat& stat) {
    //http://www.umich.edu/~eecs381/handouts/formatting.pdf
    //http://stackoverflow.com/questions/1532640/which-iomanip-manipulators-are-sticky
    std::ios_base::fmtflags current_flags = s.flags(); //save previous format flags
    std::streamsize current_precision = s.precision(); // save previous precision setting

    s << std::fixed << std::setprecision(2);
    s << read_count << ":";
    s << (float) sum_map_qualities / read_count << ":";
    s << "." << ":";   //this is for the base qualities. Stupid that this is in the base class...
    s << (float) sum_single_ended_map_qualities / read_count << ":";
    s << num_plus_strand << ":";
    s << num_minus_strand << ":";
    s << (float) sum_event_location / read_count << ":";
    s << (float) sum_number_of_mismatches / read_count << ":";
    s << (float) sum_of_mismatch_qualities / read_count << ":";
    s << num_q2_reads << ":";
    s << (float) sum_q2_distance / num_q2_reads << ":";
    s << (float) sum_of_clipped_lengths / read_count << ":";
    s << (float) sum_3p_distance / read_count;

    s.flags(current_flags); //save previous format flags
    s.precision(current_precision); // save previous precision setting
    return s;
}
