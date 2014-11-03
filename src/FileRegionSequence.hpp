#include "IRegionSequence.hpp"

#include <fstream>
#include <string>

class FileRegionSequence : public IRegionSequence {
public:
    explicit FileRegionSequence(std::string path);

    bool next(std::string& region);

private:
    std::string path_;
    std::ifstream in_;

    std::string buffer_;
};
