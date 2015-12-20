#include "bamrc/IndelQueueEntry.hpp"
#include "bamrc/ReadWarnings.hpp"
#include "bamrc/BasicStat.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <memory>

//FIXME This is required for all things in the test library
//using the ReadWarnings class.
std::auto_ptr<ReadWarnings> WARN;

using namespace std;

TEST(IndelQueueEntry, parameterized_constructor) {
    BasicStat indel_stat(true);
    IndelQueueEntry new_entry(1, 20, indel_stat, "-AA");
    ASSERT_EQ(1, new_entry.tid);
    ASSERT_EQ(20, new_entry.pos);
    ASSERT_EQ("-AA", new_entry.allele);
}

TEST(IndelQueueEntry, stream_output) {
    stringstream stat_stream;
    stat_stream << "-AA:"; 
    BasicStat indel_stat(true);
    stat_stream << indel_stat;

    stringstream entry_stream;
    IndelQueueEntry new_entry(1, 20, indel_stat, "-AA");
    entry_stream << new_entry;
    ASSERT_EQ(stat_stream.str(), entry_stream.str());
}
