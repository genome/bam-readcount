#include "bamrc/ReadWarnings.hpp"

#include <iterator>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;

namespace {
    struct Results {
        Results(std::stringstream& s)
            : total(0)
            , num_sm(0)
            , num_nm(0)
            , num_zm(0)
        {
            string line;
            while (getline(s, line)) {
                ++total;
                if (line.find("SM tag") != string::npos)
                    ++num_sm;
                else if (line.find("NM tag") != string::npos)
                    ++num_nm;
                else if (line.find("generated tag") != string::npos)
                    ++num_zm;
            }
        }

        int total;
        int num_sm;
        int num_nm;
        int num_zm;
    };
}

TEST(ReadWarnings, unlimited) {
    stringstream ss;
    ReadWarnings w(ss, -1);

    for (int i = 0; i < 100; ++i) {
        w.warn(ReadWarnings::SM_TAG_MISSING, "x");
        w.warn(ReadWarnings::NM_TAG_MISSING, "y");
        w.warn(ReadWarnings::Zm_TAG_MISSING, "z");
    }

    Results res(ss);
    ASSERT_EQ(300, res.total);
    ASSERT_EQ(100, res.num_sm);
    ASSERT_EQ(100, res.num_nm);
    ASSERT_EQ(100, res.num_zm);
}

TEST(ReadWarnings, fiveEach) {
    stringstream ss;
    ReadWarnings w(ss, 5);

    for (int i = 0; i < 100; ++i) {
        w.warn(ReadWarnings::SM_TAG_MISSING, "x");
        w.warn(ReadWarnings::NM_TAG_MISSING, "y");
        w.warn(ReadWarnings::Zm_TAG_MISSING, "z");
    }

    Results res(ss);
    // there will be one extra line for each warning type mentioning that it
    // is now disabled
    ASSERT_EQ(15 + 3, res.total);
    ASSERT_EQ(5, res.num_sm);
    ASSERT_EQ(5, res.num_nm);
    ASSERT_EQ(5, res.num_zm);

}
