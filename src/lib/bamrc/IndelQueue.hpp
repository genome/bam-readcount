#pragma once

#include "bamrc/BasicStat.hpp"
#include "bamrc/IndelQueueEntry.hpp"

#include <queue>
#include <ostream>

typedef std::queue<IndelQueueEntry> indel_queue_t;

struct IndelQueue {
    indel_queue_t queue;
    void push(IndelQueueEntry entry) { queue.push(entry); };
    int process(uint32_t tid, uint32_t pos, std::ostream& stream);
};
