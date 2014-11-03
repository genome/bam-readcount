#include "BamReader.hpp"
#include "BamFilter.hpp"

#include <boost/format.hpp>

#include <cassert>
#include <stdexcept>

using boost::format;

BamReader::BamReader(std::string path)
    : path_(std::move(path))
    , in_(samopen(path_.c_str(), "rb", 0))
    , index_(bam_index_load(path_.c_str()))
    , iter_(0)
    , total_(0)
    , filtered_(0)
{
    if (!in_ || !in_->header)
        throw std::runtime_error(str(format("Failed to open samfile %1%") % path_));

    if (!in_->x.bam)
        throw std::runtime_error(str(format("%1% is not a valid bam file") % path_));

    if (!index_)
        throw std::runtime_error(str(format("Failed to load bam index for %1%") % path_));

    set_region(0, 0, -1);
}

BamReader::~BamReader() {
    if (in_)
        samclose(in_);

    if (index_)
        bam_index_destroy(index_);

    if (iter_)
        bam_iter_destroy(iter_);
}

void BamReader::set_filter(BamFilter* filter) {
    filter_ = filter;
}

std::vector<std::string> const& BamReader::sequence_names() const {
    if (sequence_names_.empty()) {
        auto hdr = header();
        sequence_names_.assign(hdr->target_name, hdr->target_name + hdr->n_targets);
    }
    return sequence_names_;
}

void BamReader::set_region(std::string const& region) {
    int tid, beg, end;
    bam_parse_region(in_->header, region.c_str(), &tid, &beg, &end);
    set_region(tid, beg, end);
}

void BamReader::set_region(int tid, int start, int stop) {
    if (iter_)
        bam_iter_destroy(iter_);

    if (stop < 0)
        stop = header()->target_len[tid];

    iter_ = bam_iter_query(index_, tid, start, stop);
}

bool BamReader::next(bam1_t* entry) {
    int rv;
    while ((rv = bam_iter_read(in_->x.bam, iter_, entry)) >= 0) {
        ++total_;
        if (!filter_ || filter_->want_entry(entry))
            break;
        ++filtered_;
    }

    // From the samtools source code:
    // (for bam_iter_read)
    // rv >= 0 -> success
    // rv == -1: normal eof
    // rv < -1: error
    if (rv < -1) {
        throw std::runtime_error(str(format(
            "Error while reading bam file %1% (bam_read1 returned %2%; "
            "probably a truncated file)."
            ) % path() % rv));
    }
    return rv >= 0;
}

bam_header_t const* BamReader::header() const {
    return in_->header;
}

bam_header_t* BamReader::header() {
    return in_->header;
}
