#include "bamrc/IndelQueueEntry.hpp"
#include "bamrc/IndelQueue.hpp"
#include "bamrc/ReadWarnings.hpp"

#include <gtest/gtest.h>

#include <memory>

std::auto_ptr<ReadWarnings> WARN;

using namespace std;

TEST(IndelQueue, push) {
    IndelQueue test_queue;
    IndelQueueEntry entry;
    ASSERT_TRUE(test_queue.queue.empty());
    test_queue.push(entry);
    ASSERT_EQ(1, test_queue.queue.size());
}
/*
    
TEST(IndelQueue, process_irrelevant) {

}

TEST(IndelQueue, process_relevant) {

}

TEST(IndelQueue, process_new_chromosome) {

}
*/
