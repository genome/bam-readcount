#include "IRegionSequence.hpp"

#include <vector>
#include <string>

class StaticRegionSequence : public IRegionSequence {
public:
    explicit StaticRegionSequence(std::vector<std::string> rs);

    bool next(std::string& region);

private:
    std::vector<std::string> regions_;
    std::vector<std::string>::const_iterator iter_;
};
