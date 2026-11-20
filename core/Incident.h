#ifndef INCIDENT_H
#define INCIDENT_H

#include <string>

enum IncidentStatus { STATUS_REPORTED = 0, STATUS_PENDING = 1 };

// One emergency incident. Exactly ONE object exists per active incident.
// It is allocated with new by the engine and OWNED by the BST.
// The hash table and the heap only keep copies of the pointer.
struct Incident {
    int id;
    int junctionId;
    std::string category;   // MEDICAL, FIRE or CRIME
    int severity;           // 1..5 (5 = most severe)
    int timestamp;          // seconds, smaller = earlier
    IncidentStatus status;

    Incident() : id(0), junctionId(0), category(""), severity(0), timestamp(0),
                 status(STATUS_REPORTED) {}
};

// Priority comparator: returns true when incident a is ABOVE incident b.
//  1) higher severity first
//  2) if equal severity: smaller timestamp first
//  3) if still equal: smaller incident ID first
inline bool higherPriority(const Incident* a, const Incident* b) {
    if (a->severity != b->severity) return a->severity > b->severity;
    if (a->timestamp != b->timestamp) return a->timestamp < b->timestamp;
    return a->id < b->id;
}

#endif
