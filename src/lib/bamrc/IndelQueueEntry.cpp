#include "bamrc/IndelQueueEntry.hpp"

std::ostream& operator<<(std::ostream& s, const IndelQueueEntry& entry) { 
    s << entry.allele;
    s << ":";
    s << entry.indel_stats; 
    return s; 
}
