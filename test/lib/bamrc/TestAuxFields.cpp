#include "bamrc/auxfields.hpp"

#include <sstream>
#include <string>
#include <gtest/gtest.h>

using namespace std;

TEST(AuxFields, zm) {
    aux_zm_t zm;
    zm.sum_of_mismatch_qualities = 1;
    zm.clipped_length = 2;
    zm.left_clip = 3;
    zm.three_prime_index = 4;
    zm.q2_pos = 5;

    string s = zm.to_string();
    ASSERT_EQ("1 2 3 4 5", s);

    aux_zm_t zm2 = aux_zm_t::from_string(s.c_str());

    ASSERT_EQ(1, zm2.sum_of_mismatch_qualities);
    ASSERT_EQ(2, zm2.clipped_length);
    ASSERT_EQ(3, zm.left_clip);
    ASSERT_EQ(4, zm.three_prime_index);
    ASSERT_EQ(5, zm.q2_pos);
}


TEST(AuxFields, zmNegative) {
    aux_zm_t zm;
    zm.sum_of_mismatch_qualities = -1;
    zm.clipped_length = -2;
    zm.left_clip = -3;
    zm.three_prime_index = -4;
    zm.q2_pos = -5;

    string s = zm.to_string();
    ASSERT_EQ("-1 -2 -3 -4 -5", s);

    aux_zm_t zm2 = aux_zm_t::from_string(s.c_str());

    ASSERT_EQ(-1, zm2.sum_of_mismatch_qualities);
    ASSERT_EQ(-2, zm2.clipped_length);
    ASSERT_EQ(-3, zm.left_clip);
    ASSERT_EQ(-4, zm.three_prime_index);
    ASSERT_EQ(-5, zm.q2_pos);
}
