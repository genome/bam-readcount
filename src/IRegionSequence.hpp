#pragma once

#include <string>

class IRegionSequence {
public:
    virtual ~IRegionSequence() {}

    virtual bool next(std::string& region) = 0;
};
