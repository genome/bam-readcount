#include "bamrc/BasicStat.hpp"
#include "bamrc/ReadWarnings.hpp"
#include "bamrc/auxfields.hpp"

#include "htslib/sam.h"

#include <gtest/gtest.h>

#include <cstring>
#include <sstream>
#include <string>

using namespace std;

namespace {
    char const* HEADER = "@SQ\tSN:chr1\tLN:1000\n";

    // Owns a header and a bam1_t parsed from one SAM line.
    struct Read {
        sam_hdr_t* hdr;
        bam1_t* b;

        explicit Read(string const& sam) {
            hdr = sam_hdr_parse(strlen(HEADER), HEADER);
            b = bam_init1();
            kstring_t ks = {sam.size(), sam.size() + 1, strdup(sam.c_str())};
            int rv = sam_parse1(&ks, hdr, b);
            free(ks.s);
            if (rv < 0) {
                throw std::runtime_error("bad SAM line: " + sam);
            }
        }

        ~Read() {
            bam_destroy1(b);
            sam_hdr_destroy(hdr);
        }

        void add_zm(int mismatch_quals, int clipped_length, int left_clip,
                int three_prime_index, int q2_pos) {
            aux_zm_t zm;
            zm.sum_of_mismatch_qualities = mismatch_quals;
            zm.clipped_length = clipped_length;
            zm.left_clip = left_clip;
            zm.three_prime_index = three_prime_index;
            zm.q2_pos = q2_pos;
            string s = zm.to_string();
            bam_aux_append(b, "Zm", 'Z', s.size() + 1, (uint8_t*)s.c_str());
        }

        bam_pileup1_t at(int qpos) const {
            bam_pileup1_t p;
            memset(&p, 0, sizeof(p));
            p.b = b;
            p.qpos = qpos;
            return p;
        }
    };

    // 10 bp read, base qualities 30 ('?') except the last two which are Q2 ('#')
    string sam_line(int flag, int mapq, string const& tags = "") {
        stringstream ss;
        ss << "r1\t" << flag << "\tchr1\t101\t" << mapq << "\t10M\t*\t0\t0\t"
            << "ACGTACGTAC\t" << "????????##";
        if (!tags.empty()) ss << "\t" << tags;
        return ss.str();
    }

    string format(BasicStat const& s) {
        stringstream ss;
        ss << s;
        return ss.str();
    }

    struct BasicStatTest : public ::testing::Test {
        stringstream warnings;
        void SetUp() { WARN.reset(new ReadWarnings(warnings, -1)); }
        void TearDown() { WARN.reset(); }
    };
}

TEST_F(BasicStatTest, emptyStatFormatsAsZeros) {
    BasicStat s;
    EXPECT_EQ("0:0.00:0.00:0.00:0:0:0.00:0.00:0.00:0:0.00:0.00:0.00", format(s));
    BasicStat indel(true);
    EXPECT_EQ("0:0.00:0.00:0.00:0:0:0.00:0.00:0.00:0:0.00:0.00:0.00", format(indel));
}

TEST_F(BasicStatTest, forwardReadWithZm) {
    Read r(sam_line(0, 50, "NM:i:1"));
    r.add_zm(20, 10, 0, 7, 7);
    bam_pileup1_t p = r.at(2);

    BasicStat s;
    s.process_read(&p);

    EXPECT_EQ(1u, s.read_count);
    EXPECT_EQ(50u, s.sum_map_qualities);
    EXPECT_EQ(50u, s.sum_single_ended_map_qualities); // not a proper pair: uses MAPQ
    EXPECT_EQ(1u, s.num_plus_strand);
    EXPECT_EQ(0u, s.num_minus_strand);
    EXPECT_EQ(30u, s.sum_base_qualities);
    EXPECT_EQ(20u, s.sum_of_mismatch_qualities);
    EXPECT_EQ(10u, s.sum_of_clipped_lengths);
    EXPECT_EQ(1u, s.num_q2_reads);
    EXPECT_FLOAT_EQ(0.5f, s.sum_q2_distance);      // |2 - 7| / 10
    EXPECT_FLOAT_EQ(0.5f, s.sum_3p_distance);      // |2 - 7| / 10
    EXPECT_FLOAT_EQ(0.4f, s.sum_event_location);   // 1 - |2 - 5| / 5
    EXPECT_FLOAT_EQ(0.1f, s.sum_number_of_mismatches); // NM 1 / 10
    EXPECT_EQ("", warnings.str());

    EXPECT_EQ("1:50.00:30.00:50.00:1:0:0.40:0.10:20.00:1:0.50:10.00:0.50", format(s));
}

TEST_F(BasicStatTest, reverseReadNoQ2) {
    Read r(sam_line(BAM_FREVERSE, 60, "NM:i:0"));
    r.add_zm(0, 10, 0, 0, -1);
    bam_pileup1_t p = r.at(5);

    BasicStat s;
    s.process_read(&p);

    EXPECT_EQ(0u, s.num_plus_strand);
    EXPECT_EQ(1u, s.num_minus_strand);
    EXPECT_EQ(0u, s.num_q2_reads);
    EXPECT_FLOAT_EQ(0.0f, s.sum_q2_distance);
    EXPECT_FLOAT_EQ(0.5f, s.sum_3p_distance);      // |5 - 0| / 10
    EXPECT_EQ("1:60.00:30.00:60.00:0:1:1.00:0.00:0.00:0:0.00:10.00:0.50", format(s));
}

TEST_F(BasicStatTest, softClipAdjustsEventLocation) {
    Read r("r1\t0\tchr1\t101\t60\t2S8M\t*\t0\t0\tACGTACGTAC\t??????????\tNM:i:0");
    r.add_zm(0, 8, 2, 9, -1);
    bam_pileup1_t p = r.at(6);   // 4 bases into the aligned part: centre of 8

    BasicStat s;
    s.process_read(&p);
    EXPECT_FLOAT_EQ(1.0f, s.sum_event_location);
    EXPECT_EQ(8u, s.sum_of_clipped_lengths);
}

TEST_F(BasicStatTest, missingZmWarnsAndSkipsReadMetrics) {
    Read r(sam_line(0, 40, "NM:i:0"));
    bam_pileup1_t p = r.at(0);

    BasicStat s;
    s.process_read(&p);
    EXPECT_EQ(1u, s.read_count);
    EXPECT_EQ(0u, s.sum_of_clipped_lengths);
    EXPECT_FLOAT_EQ(0.0f, s.sum_event_location);
    EXPECT_NE(string::npos, warnings.str().find("Couldn't find the generated tag"));
}

TEST_F(BasicStatTest, missingNmWarns) {
    Read r(sam_line(0, 40));
    r.add_zm(0, 10, 0, 9, -1);
    bam_pileup1_t p = r.at(0);

    BasicStat s;
    s.process_read(&p);
    EXPECT_FLOAT_EQ(0.0f, s.sum_number_of_mismatches);
    EXPECT_NE(string::npos, warnings.str().find("number of mismatches"));
}

TEST_F(BasicStatTest, properPairUsesSmTag) {
    Read r(sam_line(BAM_FPAIRED | BAM_FPROPER_PAIR, 60, "NM:i:0\tSM:i:37"));
    r.add_zm(0, 10, 0, 9, -1);
    bam_pileup1_t p = r.at(0);

    BasicStat s;
    s.process_read(&p);
    EXPECT_EQ(37u, s.sum_single_ended_map_qualities);
    EXPECT_EQ("", warnings.str());
}

TEST_F(BasicStatTest, properPairWithoutSmWarns) {
    Read r(sam_line(BAM_FPAIRED | BAM_FPROPER_PAIR, 60, "NM:i:0"));
    r.add_zm(0, 10, 0, 9, -1);
    bam_pileup1_t p = r.at(0);

    BasicStat s;
    s.process_read(&p);
    EXPECT_EQ(0u, s.sum_single_ended_map_qualities);
    EXPECT_NE(string::npos, warnings.str().find("single-end mapping quality"));
}

TEST_F(BasicStatTest, indelStatHasNoBaseQuality) {
    Read r(sam_line(0, 60, "NM:i:1"));
    r.add_zm(0, 10, 0, 9, -1);
    bam_pileup1_t p = r.at(3);

    BasicStat s(true);
    s.process_read(&p);
    EXPECT_EQ(0u, s.sum_base_qualities);
    string f = format(s);
    EXPECT_EQ("1:60.00:0.00:", f.substr(0, 13));
}

TEST_F(BasicStatTest, averagesOverSeveralReads) {
    Read a(sam_line(0, 60, "NM:i:0"));
    a.add_zm(10, 10, 0, 9, -1);
    Read b(sam_line(BAM_FREVERSE, 21, "NM:i:2"));
    b.add_zm(35, 10, 0, 0, -1);
    Read c(sam_line(0, 0, "NM:i:1"));
    c.add_zm(0, 10, 0, 9, -1);

    BasicStat s;
    bam_pileup1_t pa = a.at(1), pb = b.at(1), pc = c.at(1);
    s.process_read(&pa);
    s.process_read(&pb);
    s.process_read(&pc);

    EXPECT_EQ(3u, s.read_count);
    EXPECT_EQ(2u, s.num_plus_strand);
    EXPECT_EQ(1u, s.num_minus_strand);
    // (60 + 21 + 0) / 3 = 27.00; mismatch quals (10 + 35 + 0) / 3 = 15.00
    string f = format(s);
    EXPECT_EQ("3:27.00:30.00:27.00:2:1:", f.substr(0, 24));
    EXPECT_NE(string::npos, f.find(":15.00:"));
}

TEST_F(BasicStatTest, roundsToTwoDecimals) {
    Read a(sam_line(0, 1, "NM:i:0"));
    a.add_zm(0, 10, 0, 9, -1);
    Read b(sam_line(0, 0, "NM:i:0"));
    b.add_zm(0, 10, 0, 9, -1);
    Read c(sam_line(0, 0, "NM:i:0"));
    c.add_zm(0, 10, 0, 9, -1);

    BasicStat s;
    bam_pileup1_t pa = a.at(0), pb = b.at(0), pc = c.at(0);
    s.process_read(&pa);
    s.process_read(&pb);
    s.process_read(&pc);
    EXPECT_EQ("3:0.33:", format(s).substr(0, 7));
}

TEST_F(BasicStatTest, formattingDoesNotLeakStreamState) {
    Read r(sam_line(0, 60, "NM:i:0"));
    r.add_zm(0, 10, 0, 9, -1);
    bam_pileup1_t p = r.at(0);
    BasicStat s;
    s.process_read(&p);

    stringstream ss;
    ss << 1.23456 << " " << s << " " << 1.23456;
    string out = ss.str();
    EXPECT_EQ("1.23456 ", out.substr(0, 8));
    EXPECT_EQ(" 1.23456", out.substr(out.size() - 8));
}
