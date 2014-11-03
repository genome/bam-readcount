
#pragma once

#include <bam.h>

#include <cstdint>

class BamEntry {
public:
    BamEntry()
        : data(bam_init1())
    {
    }

    BamEntry(BamEntry&& that) {
        data = that.data;
        if (this != &that)
            that.data = bam_init1();
    }

    BamEntry(BamEntry const&) = delete;
    BamEntry& operator=(BamEntry const&) = delete;

    ~BamEntry() {
        if (data)
            bam_destroy1(data);
    }

    // conversion to bam1_t
    operator bam1_t*() { return data; }
    operator bam1_t const*() const { return data; }
    bam1_t* operator->() { return data; }
    bam1_t const* operator->() const { return data; }

private:
    bam1_t* data;
};
