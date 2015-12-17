#pragma once

#include "bamrc/BasicStat.hpp"

#include <string>
#include <ostream>

struct IndelQueueEntry {
    uint32_t tid;
    uint32_t pos;
    BasicStat indel_stats;
    std::string allele;
    IndelQueueEntry() : tid(0), pos(0), indel_stats(), allele() {}
    IndelQueueEntry(uint32_t tid, uint32_t pos, BasicStat indel_stats, std::string allele) : tid(tid), pos(pos), indel_stats(indel_stats), allele(allele) {}
};

std::ostream& operator<<(std::ostream& s, const IndelQueueEntry& entry); 
