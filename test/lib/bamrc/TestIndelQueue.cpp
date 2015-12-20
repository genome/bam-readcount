#include "bamrc/IndelQueueEntry.hpp"
#include "bamrc/IndelQueue.hpp"
#include "bamrc/ReadWarnings.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <memory>

using namespace std;

TEST(IndelQueue, push) {
    IndelQueue test_queue;
    IndelQueueEntry entry;
    ASSERT_TRUE(test_queue.queue.empty());
    test_queue.push(entry);
    ASSERT_EQ(1, test_queue.queue.size());
}

    
TEST(IndelQueue, process_irrelevant) {
    //This test verifies the queue is cleared of sites that are passed by
    IndelQueue test_queue;

    IndelQueueEntry test1;
    test1.tid = 1;
    test1.pos = 1;

    test_queue.push(test1);

    IndelQueueEntry test2;
    test2.tid = 1;
    test2.pos = 5;

    test_queue.push(test2);

    stringstream stream;

    test_queue.process(1, 2, stream);
    ASSERT_EQ(1, test_queue.queue.size());
}

TEST(IndelQueue, process_new_chromosome) {
    //This test verifies the queue is cleared of sites when the chromosome changes
    IndelQueue test_queue;

    IndelQueueEntry test1;
    test1.tid = 1;
    test1.pos = 1;

    test_queue.push(test1);

    IndelQueueEntry test2;
    test2.tid = 1;
    test2.pos = 5;

    test_queue.push(test2);

    stringstream stream;

    test_queue.process(10, 2, stream);
    ASSERT_EQ(0, test_queue.queue.size());
}

TEST(IndelQueue, process_relevant) {
    IndelQueue test_queue;

    IndelQueueEntry test1;
    test1.tid = 1;
    test1.pos = 1;

    test_queue.push(test1);

    IndelQueueEntry test2;
    test2.tid = 1;
    test2.pos = 5;
    test2.indel_stats.read_count = 10;

    test_queue.push(test2);

    stringstream stream;
    ASSERT_EQ(0, stream.str().size());

    int depth = test_queue.process(1, 5, stream);
    ASSERT_EQ(0, test_queue.queue.size());
    ASSERT_EQ(10, depth);
    ASSERT_NE(0, stream.str().size());

}
