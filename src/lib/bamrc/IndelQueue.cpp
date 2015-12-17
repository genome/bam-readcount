#include "bamrc/IndelQueue.hpp"

int IndelQueue::process(uint32_t tid, uint32_t pos, std::ostream& stream) {
    int extra_depth = 0;
    while(!queue.empty() && ( (queue.front().tid == tid && queue.front().pos < pos) || (queue.front().tid != tid))) {
        queue.pop();
    }

    while(!queue.empty() && queue.front().tid == tid && queue.front().pos == pos) {
        stream << "\t" << queue.front();
        extra_depth += queue.front().indel_stats.read_count;
        queue.pop();
    }
    return extra_depth;
}

