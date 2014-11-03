#pragma once

#include "Options.hpp"

struct BamFilter {
    BamFilter(int min_mapq, int required_flags, int forbidden_flags)
        : min_mapq(min_mapq)
        , required_flags(required_flags)
        , forbidden_flags(forbidden_flags)
    {}

    template<typename T>
    bool want_entry(T const& e) const {
        int mapq = e->core.qual;
        int flag = e->core.flag;
        return mapq >= min_mapq
            && (flag & required_flags) == required_flags
            && (flag & forbidden_flags) == 0;
    }

    int min_mapq;
    int required_flags;
    int forbidden_flags;
};
