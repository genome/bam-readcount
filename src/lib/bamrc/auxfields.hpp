#pragma once

#include <string>
#include <sstream>
#include <cerrno>
#include <climits>
#include <cstdlib>

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
        aux_zm_t zm;
        int* fields[] = {&zm.sum_of_mismatch_qualities, &zm.clipped_length,
            &zm.left_clip, &zm.three_prime_index, &zm.q2_pos};
        const char* next = data;

        // Parse the five integers without constructing a stream for every base.
        for (int i = 0; i < 5; ++i) {
            char* end;
            errno = 0;
            long value = std::strtol(next, &end, 10);
            if (end == next || errno == ERANGE || value < INT_MIN || value > INT_MAX) {
                break;
            }
            *fields[i] = static_cast<int>(value);
            next = end;
            if (i == 4) {
                return zm;
            }
        }

        // Keep the original stream behavior for malformed tags.
        std::stringstream ss(data);
        ss >> zm.sum_of_mismatch_qualities
            >> zm.clipped_length
            >> zm.left_clip
            >> zm.three_prime_index
            >> zm.q2_pos;

        return zm;
    }
};
