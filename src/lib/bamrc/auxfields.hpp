#pragma once

#include <string>
#include <sstream>

struct aux_zm_t {
    int sum_of_mismatch_qualities;
    int clipped_length;
    int left_clip;
    int three_prime_index;
    int q2_pos;

    std::string to_string() const {
        std::stringstream ss;
        ss << sum_of_mismatch_qualities
            << " " << clipped_length
            << " " << left_clip
            << " " << three_prime_index
            << " " << q2_pos;
        return ss.str();
    }

    static aux_zm_t from_string(char const* data) {
        std::stringstream ss(data);

        aux_zm_t zm;
        ss >> zm.sum_of_mismatch_qualities
            >> zm.clipped_length
            >> zm.left_clip
            >> zm.three_prime_index
            >> zm.q2_pos;

        return zm;
    }
};
