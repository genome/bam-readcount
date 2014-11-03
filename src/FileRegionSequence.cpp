#include "FileRegionSequence.hpp"

#include <boost/format.hpp>

#include <sstream>
#include <stdexcept>

using boost::format;

FileRegionSequence::FileRegionSequence(std::string path)
    : path_(std::move(path))
    , in_(path_)
{
    if (!in_.is_open()) {
        throw std::runtime_error(str(format(
            "Failed to open region file %1%"
            ) % path_));
    }
}

bool FileRegionSequence::next(std::string& region) {
    bool rv = false;
    std::string ref_name;
    int beg, end;
    while ((rv = std::getline(in_, buffer_))) {
        std::stringstream line(buffer_);
        if (line >> ref_name >> beg >> end) {
            std::stringstream ss;
            ss << ref_name << ":" << beg << "-" << end;
            region = ss.str();
            break;
        }
    }

    return rv;
}
