#include "bamrc/auxfields.hpp"

#include <climits>
#include <cstdlib>
#include <sstream>
#include <string>
#include <gtest/gtest.h>

using namespace std;

namespace {
    // The original stream-based parser, kept here as a reference
    // implementation for differential testing of aux_zm_t::from_string.
    // Returns the number of fields successfully extracted.
    int legacy_parse(char const* data, aux_zm_t& zm) {
        std::stringstream ss(data);
        int* fields[] = {&zm.sum_of_mismatch_qualities, &zm.clipped_length,
            &zm.left_clip, &zm.three_prime_index, &zm.q2_pos};
        int n = 0;
        for (int i = 0; i < 5; ++i) {
            if (!(ss >> *fields[i])) {
                return n;
            }
            ++n;
        }
        return n;
    }

    aux_zm_t make(int a, int b, int c, int d, int e) {
        aux_zm_t zm;
        zm.sum_of_mismatch_qualities = a;
        zm.clipped_length = b;
        zm.left_clip = c;
        zm.three_prime_index = d;
        zm.q2_pos = e;
        return zm;
    }

    void expect_fields(aux_zm_t const& zm, int a, int b, int c, int d, int e) {
        EXPECT_EQ(a, zm.sum_of_mismatch_qualities);
        EXPECT_EQ(b, zm.clipped_length);
        EXPECT_EQ(c, zm.left_clip);
        EXPECT_EQ(d, zm.three_prime_index);
        EXPECT_EQ(e, zm.q2_pos);
    }
}

TEST(AuxFields, zm) {
    aux_zm_t zm = make(1, 2, 3, 4, 5);

    string s = zm.to_string();
    ASSERT_EQ("1 2 3 4 5", s);

    aux_zm_t zm2 = aux_zm_t::from_string(s.c_str());
    expect_fields(zm2, 1, 2, 3, 4, 5);
}


TEST(AuxFields, zmNegative) {
    aux_zm_t zm = make(-1, -2, -3, -4, -5);

    string s = zm.to_string();
    ASSERT_EQ("-1 -2 -3 -4 -5", s);

    aux_zm_t zm2 = aux_zm_t::from_string(s.c_str());
    expect_fields(zm2, -1, -2, -3, -4, -5);
}

TEST(AuxFields, zmTypicalValues) {
    // q2_pos and three_prime_index are -1 when there is no Q2 run
    aux_zm_t zm2 = aux_zm_t::from_string("0 100 0 99 -1");
    expect_fields(zm2, 0, 100, 0, 99, -1);
}

TEST(AuxFields, zmIntLimits) {
    aux_zm_t zm = make(INT_MAX, INT_MIN, 0, INT_MAX, INT_MIN);
    aux_zm_t zm2 = aux_zm_t::from_string(zm.to_string().c_str());
    expect_fields(zm2, INT_MAX, INT_MIN, 0, INT_MAX, INT_MIN);
}

TEST(AuxFields, zmWhitespaceVariants) {
    expect_fields(aux_zm_t::from_string("1\t2\t3\t4\t5"), 1, 2, 3, 4, 5);
    expect_fields(aux_zm_t::from_string("  1   2\n3 \t 4  5  "), 1, 2, 3, 4, 5);
    expect_fields(aux_zm_t::from_string("\n1 2 3 4 5\n"), 1, 2, 3, 4, 5);
}

TEST(AuxFields, zmLeadingPlusAndZeros) {
    expect_fields(aux_zm_t::from_string("+1 +2 007 -0 +0"), 1, 2, 7, 0, 0);
}

TEST(AuxFields, zmTrailingJunkIgnored) {
    expect_fields(aux_zm_t::from_string("1 2 3 4 5 6 7"), 1, 2, 3, 4, 5);
    expect_fields(aux_zm_t::from_string("1 2 3 4 5xyz"), 1, 2, 3, 4, 5);
}

TEST(AuxFields, zmMalformedMatchesLegacyParser) {
    // For malformed input only the fields the legacy parser actually extracted
    // are defined; those (and, in C++11, a failed field set to 0 or clamped)
    // must agree.
    char const* inputs[] = {
        "", "   ", "1 2 3", "1 2 3 4", "abc", "1 2 x 4 5", "1 2 3 4 x",
        "99999999999 2 3 4 5", "1 2 3 4 -99999999999", "1,2,3,4,5",
        "1 2 3 4 5.5", "- 1 2 3 4 5", "0x10 2 3 4 5",
    };
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        aux_zm_t expected = make(-7, -7, -7, -7, -7);
        int n = legacy_parse(inputs[i], expected);
        aux_zm_t got = aux_zm_t::from_string(inputs[i]);
        int* e[] = {&expected.sum_of_mismatch_qualities, &expected.clipped_length,
            &expected.left_clip, &expected.three_prime_index, &expected.q2_pos};
        int* g[] = {&got.sum_of_mismatch_qualities, &got.clipped_length,
            &got.left_clip, &got.three_prime_index, &got.q2_pos};
        for (int f = 0; f < n; ++f) {
            EXPECT_EQ(*e[f], *g[f]) << "input '" << inputs[i] << "' field " << f;
        }
    }
}

TEST(AuxFields, zmDifferentialRandom) {
    // Random well-formed inputs with varied separators must parse exactly as
    // the legacy stream parser does.
    srand(113);
    char const* seps[] = {" ", "  ", "\t", " \t ", "\n"};
    for (int iter = 0; iter < 10000; ++iter) {
        std::stringstream ss;
        if (rand() % 4 == 0) ss << seps[rand() % 5];
        for (int f = 0; f < 5; ++f) {
            long v;
            switch (rand() % 4) {
                case 0: v = rand() % 200 - 100; break;
                case 1: v = rand(); break;
                case 2: v = -(long)rand(); break;
                default: v = (rand() % 2) ? INT_MAX - rand() % 3 : INT_MIN + rand() % 3; break;
            }
            if (f) ss << seps[rand() % 5];
            ss << v;
        }
        if (rand() % 4 == 0) ss << seps[rand() % 5];
        std::string s = ss.str();

        aux_zm_t expected;
        ASSERT_EQ(5, legacy_parse(s.c_str(), expected)) << s;
        aux_zm_t got = aux_zm_t::from_string(s.c_str());
        ASSERT_EQ(expected.sum_of_mismatch_qualities, got.sum_of_mismatch_qualities) << s;
        ASSERT_EQ(expected.clipped_length, got.clipped_length) << s;
        ASSERT_EQ(expected.left_clip, got.left_clip) << s;
        ASSERT_EQ(expected.three_prime_index, got.three_prime_index) << s;
        ASSERT_EQ(expected.q2_pos, got.q2_pos) << s;
    }
}

TEST(AuxFields, zmRoundTripRandom) {
    srand(1395);
    for (int iter = 0; iter < 10000; ++iter) {
        aux_zm_t zm = make(rand() - RAND_MAX / 2, rand() % 1000, rand() % 200,
            rand() % 300 - 1, rand() % 300 - 1);
        aux_zm_t zm2 = aux_zm_t::from_string(zm.to_string().c_str());
        ASSERT_EQ(zm.to_string(), zm2.to_string());
    }
}
