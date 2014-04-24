#pragma once

#include <stdint.h>
#include <cstddef>
#include <iostream>
#include <vector>
#include <string>
#include <memory>

class ReadWarnings {
public:
    enum WarningType {
        SM_TAG_MISSING = 0,
        NM_TAG_MISSING,
        Zm_TAG_MISSING,
        LIBRARY_UNAVAILABLE,
        N_WARNING_TYPES
    };

public:
    ReadWarnings(std::ostream& stream, int64_t max_count_per_type)
        : _stream(stream)
        , _max_count_per_type(max_count_per_type)
        , _counts(N_WARNING_TYPES, 0)
        , _messages(N_WARNING_TYPES)
    {
        _messages[SM_TAG_MISSING] = "Couldn't find single-end mapping quality. "
            "Check to see if the SM tag is in BAM.";

        _messages[NM_TAG_MISSING] = "Couldn't find number of mismatches. "
            "Check to see if the NM tag is in BAM.";

        _messages[Zm_TAG_MISSING] = "Couldn't find the generated tag.";

        _messages[LIBRARY_UNAVAILABLE] = "Library unavailable. "
            "Check to make sure the LB tag is present in the @RG entries of the header.";
    }

    void warn(WarningType type, char const* read_name) {
        ++_counts[type];
        if (_max_count_per_type >= 0 && _counts[type] > _max_count_per_type) {
            return;
        }
        _stream << "WARNING: In read " << read_name << ": " << _messages[type] << "\n";

        if (_max_count_per_type >= 0 && _counts[type] == _max_count_per_type) {
            _stream << "The previous warning has been emitted "
                << _counts[type] << " times and will be disabled.\n";
        }
    }

private:
    std::ostream& _stream;
    int64_t _max_count_per_type;
    std::vector<int64_t> _counts;
    std::vector<std::string> _messages;
};


extern std::auto_ptr<ReadWarnings> WARN;
