#pragma once

#include <sam.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct BamFilter;

class BamReader {
public:
    explicit BamReader(std::string path);
    ~BamReader();

    std::vector<std::string> const& sequence_names() const;

    void set_filter(BamFilter* filter);
    void set_region(std::string const& region);
    void set_region(int tid, int start, int stop);
    int current_tid() const;

    bool next(bam1_t* entry);

    bam_header_t const* header() const;
    bam_header_t* header();

    std::string const& path() const { return path_; }

    std::size_t total_read() const { return total_; }
    std::size_t total_filtered() const { return filtered_; }

private:
    std::string path_;
    samfile_t* in_;
    bam_index_t* index_;
    bam_iter_t iter_;

    BamFilter* filter_;

    std::size_t total_;
    std::size_t filtered_;
    mutable std::vector<std::string> sequence_names_;
};
