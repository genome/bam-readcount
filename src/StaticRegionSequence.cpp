#include "StaticRegionSequence.hpp"

#include <boost/format.hpp>

using boost::format;

StaticRegionSequence::StaticRegionSequence(std::vector<std::string> rs)
    : regions_(std::move(rs))
    , iter_(regions_.begin())
{
}

bool StaticRegionSequence::next(std::string& region) {
    if (iter_ == regions_.end())
        return false;

    region = *iter_++;
    return true;
}
