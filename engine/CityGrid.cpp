#include "CityGrid.h"

#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Small parsing helpers (kept local to this file)
// ---------------------------------------------------------------------------
namespace {

const int MAX_ID = 999999999;

// Removes ONE trailing carriage return (Windows line endings).
std::string stripCR(const std::string& s) {
    if (!s.empty() && s[s.size() - 1] == '\r') return s.substr(0, s.size() - 1);
    return s;
}

bool isBlank(const std::string& s) {
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] != ' ' && s[i] != '\t') return false;
    return true;
}

// Removes leading and trailing spaces/tabs.
std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) b++;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t')) e--;
    return s.substr(b, e - b);
}

// Integer field: only digits 0-9, 1..9 digits, value within [lo, hi].
bool parseInt(const std::string& s, long long lo, long long hi, int& out) {
    if (s.empty() || s.size() > 9) return false;
    long long v = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] < '0' || s[i] > '9') return false;
        v = v * 10 + (s[i] - '0');
    }
    if (v < lo || v > hi) return false;
    out = (int)v;
    return true;
}

// Splits line on '|' into exactly n trimmed fields. False if the count differs.
bool splitFields(const std::string& line, int n, std::string* out) {
    int bars = 0;
    for (size_t i = 0; i < line.size(); i++)
        if (line[i] == '|') bars++;
    if (bars != n - 1) return false;

    size_t start = 0;
    for (int i = 0; i < n; i++) {
        if (i < n - 1) {
            size_t p = line.find('|', start);
            out[i] = trim(line.substr(start, p - start));
            start = p + 1;
        } else {
            out[i] = trim(line.substr(start));
        }
    }
    return true;
}

bool isJunctionType(const std::string& t) {
    return t == "HOSPITAL" || t == "FIRE" || t == "POLICE" || t == "DEPOT" || t == "PLAIN";
}

bool isCategory(const std::string& c) {
    return c == "MEDICAL" || c == "FIRE" || c == "CRIME";
}

bool validName(const std::string& n) {
    if (n.empty()) return false;
    for (size_t i = 0; i < n.size(); i++)
        if (n[i] == '|') return false;
    return true;
}

// Number of arguments a command takes, or -1 for an unknown command.
int argCount(const std::string& c) {
    if (c == "CITY_ADD") return 3;
    if (c == "CITY_FIND") return 1;
    if (c == "INC_REPORT") return 5;
    if (c == "INC_FIND") return 1;
    if (c == "INC_CLOSE") return 1;
    if (c == "REQUEST") return 1;
    if (c == "CITY_LIST" || c == "CITY_PREORDER" || c == "INC_LIST" ||
        c == "INC_PREORDER" || c == "PRIORITY_NEXT" || c == "PRIORITY_SERVE" ||
        c == "PRIORITY_COUNT" || c == "HASH_STATS")
        return 0;
    return -1;
}

// ---- visitors used with the tree traversals (they do the printing) --------
struct JunctionLister {
    void operator()(const int&, const Junction& j) const {
        std::cout << "JUNCTION " << j.id << " " << j.name << " " << j.type << "\n";
    }
};
struct JunctionPreorderPrinter {
    void operator()(const int& key, const Junction&) const { std::cout << " " << key; }
};
struct IncidentLister {
    void operator()(const int&, Incident* const& inc) const {
        std::cout << "INCIDENT " << inc->id << " junction=" << inc->junctionId
                  << " category=" << inc->category << " severity=" << inc->severity
                  << " time=" << inc->timestamp << " status="
                  << (inc->status == STATUS_PENDING ? "PENDING" : "REPORTED") << "\n";
    }
};
struct IncidentPreorderPrinter {
    void operator()(const int& key, Incident* const&) const { std::cout << " " << key; }
};

} // namespace

// ---------------------------------------------------------------------------
// Construction / helpers
// ---------------------------------------------------------------------------
CityGrid::CityGrid() {}
CityGrid::~CityGrid() {}   // members free everything (BST deletes the incidents)

void CityGrid::printJunction(const Junction& j) {
    std::cout << "JUNCTION " << j.id << " " << j.name << " " << j.type << "\n";
}

void CityGrid::printIncident(const Incident* inc) {
    std::cout << "INCIDENT " << inc->id << " junction=" << inc->junctionId
              << " category=" << inc->category << " severity=" << inc->severity
              << " time=" << inc->timestamp << " status="
              << (inc->status == STATUS_PENDING ? "PENDING" : "REPORTED") << "\n";
}

void CityGrid::commandError(int lineNo, const std::string& reason) {
    std::cout << "ERROR line " << lineNo << ": " << reason << "\n";
}

// Creates the single Incident object and registers it in BST + hash table.
void CityGrid::registerIncident(int id, int junctionId, const std::string& category,
                                int severity, int timestamp) {
    Incident* inc = new Incident();
    inc->id = id;
    inc->junctionId = junctionId;
    inc->category = category;
    inc->severity = severity;
    inc->timestamp = timestamp;
    inc->status = STATUS_REPORTED;
    incidents.insert(id, inc);   // BST owns it
    table.insert(id, inc);       // hash table only keeps the pointer
}

// The incident must already be out of the heap. Removes it from the hash table
// first, then from the BST (which deletes the object). Order matters: the id is
// copied out first so nothing touches the object after it is freed.
void CityGrid::closeIncident(Incident* inc) {
    int id = inc->id;
    table.remove(id);
    incidents.remove(id);        // frees the Incident
}

// ---------------------------------------------------------------------------
// File loading
// ---------------------------------------------------------------------------
void CityGrid::loadJunctions(std::istream& in, const std::string& fileName) {
    int lineNo = 0, accepted = 0, rejected = 0;
    std::string raw;
    while (std::getline(in, raw)) {
        lineNo++;
        std::string line = stripCR(raw);
        if (isBlank(line)) continue;

        std::string f[3];
        const char* reason = 0;
        int id = 0;

        if (!splitFields(line, 3, f))                     reason = "wrong field count";
        else if (!parseInt(f[0], 1, MAX_ID, id))          reason = "invalid junction ID";
        else if (f[1].empty())                            reason = "empty name";
        else if (!isJunctionType(f[2]))                   reason = "invalid type";
        else if (junctions.find(id))                      reason = "duplicate junction ID";

        if (reason) {
            std::cout << "ERROR " << fileName << ":" << lineNo << ": " << reason << "\n";
            rejected++;
        } else {
            junctions.insert(id, Junction(id, f[1], f[2]));
            accepted++;
        }
    }
    std::cout << "LOAD " << fileName << ": " << accepted << " accepted, "
              << rejected << " rejected\n";
}

void CityGrid::loadIncidents(std::istream& in, const std::string& fileName) {
    int lineNo = 0, accepted = 0, rejected = 0;
    std::string raw;
    while (std::getline(in, raw)) {
        lineNo++;
        std::string line = stripCR(raw);
        if (isBlank(line)) continue;

        std::string f[5];
        std::string reason;
        int id = 0, jid = 0, sev = 0, ts = 0;

        if (!splitFields(line, 5, f))                     reason = "wrong field count";
        else if (!parseInt(f[0], 1, MAX_ID, id))          reason = "invalid incident ID";
        else if (!parseInt(f[1], 1, MAX_ID, jid))         reason = "invalid junction ID";
        else if (!isCategory(f[2]))                       reason = "invalid category";
        else if (!parseInt(f[3], 1, 5, sev))              reason = "invalid severity";
        else if (!parseInt(f[4], 0, MAX_ID, ts))          reason = "invalid timestamp";
        else if (table.find(id))                          reason = "duplicate incident ID";
        else if (!junctions.find(jid)) {
            reason = "junction " + std::to_string(jid) + " not found";
        }

        if (!reason.empty()) {
            std::cout << "ERROR " << fileName << ":" << lineNo << ": " << reason << "\n";
            rejected++;
        } else {
            registerIncident(id, jid, f[2], sev, ts);
            accepted++;
        }
    }
    std::cout << "LOAD " << fileName << ": " << accepted << " accepted, "
              << rejected << " rejected\n";
}

// ---------------------------------------------------------------------------
// Script execution
// ---------------------------------------------------------------------------
void CityGrid::runScript(std::istream& in) {
    int lineNo = 0;
    std::string raw;
    while (std::getline(in, raw)) {
        lineNo++;
        std::string line = stripCR(raw);
        if (isBlank(line)) continue;
        std::string t = trim(line);
        if (t[0] == '#') continue;               // comment
        std::cout << "> " << t << "\n";
        execute(t, lineNo);
    }
}

void CityGrid::execute(const std::string& line, int lineNo) {
    // Tokenize on spaces/tabs. Only the first 8 tokens are stored, all are counted.
    const int MAXTOK = 8;
    std::string tok[MAXTOK];
    int nTok = 0;
    size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
        if (i >= line.size()) break;
        size_t start = i;
        while (i < line.size() && line[i] != ' ' && line[i] != '\t') i++;
        if (nTok < MAXTOK) tok[nTok] = line.substr(start, i - start);
        nTok++;
    }

    const std::string& cmd = tok[0];

    // Check 1: known command
    int expected = argCount(cmd);
    if (expected < 0) { commandError(lineNo, "unknown command " + cmd); return; }

    // Check 2: argument count
    if (nTok - 1 != expected) { commandError(lineNo, "wrong argument count"); return; }

    // ----------------------------------------------------------- junctions
    if (cmd == "CITY_ADD") {
        int id = 0;
        if (!parseInt(tok[1], 1, MAX_ID, id)) { commandError(lineNo, "invalid junction ID"); return; }
        if (!validName(tok[2]))               { commandError(lineNo, "invalid name"); return; }
        if (!isJunctionType(tok[3]))          { commandError(lineNo, "invalid type"); return; }
        if (junctions.find(id))               { commandError(lineNo, "duplicate junction ID"); return; }
        junctions.insert(id, Junction(id, tok[2], tok[3]));
        std::cout << "JUNCTION ADDED " << id << "\n";
        return;
    }
    if (cmd == "CITY_FIND") {
        int id = 0;
        if (!parseInt(tok[1], 1, MAX_ID, id)) { commandError(lineNo, "invalid junction ID"); return; }
        Junction* j = junctions.find(id);
        if (!j) { commandError(lineNo, "junction " + std::to_string(id) + " not found"); return; }
        printJunction(*j);
        return;
    }
    if (cmd == "CITY_LIST") {
        std::cout << "JUNCTIONS " << junctions.size() << "\n";
        JunctionLister v;
        junctions.inorder(v);
        return;
    }
    if (cmd == "CITY_PREORDER") {
        if (junctions.empty()) { std::cout << "JUNCTION PREORDER EMPTY\n"; return; }
        std::cout << "JUNCTION PREORDER";
        JunctionPreorderPrinter v;
        junctions.preorder(v);
        std::cout << "\n";
        return;
    }

    // ----------------------------------------------------------- incidents
    if (cmd == "INC_REPORT") {
        int id = 0, jid = 0, sev = 0, ts = 0;
        if (!parseInt(tok[1], 1, MAX_ID, id))  { commandError(lineNo, "invalid incident ID"); return; }
        if (!parseInt(tok[2], 1, MAX_ID, jid)) { commandError(lineNo, "invalid junction ID"); return; }
        if (!isCategory(tok[3]))               { commandError(lineNo, "invalid category"); return; }
        if (!parseInt(tok[4], 1, 5, sev))      { commandError(lineNo, "invalid severity"); return; }
        if (!parseInt(tok[5], 0, MAX_ID, ts))  { commandError(lineNo, "invalid timestamp"); return; }
        if (table.find(id))                    { commandError(lineNo, "duplicate incident ID"); return; }
        if (!junctions.find(jid)) {
            commandError(lineNo, "junction " + std::to_string(jid) + " not found");
            return;
        }
        registerIncident(id, jid, tok[3], sev, ts);
        std::cout << "INCIDENT REPORTED " << id << "\n";
        return;
    }
    if (cmd == "INC_FIND") {
        int id = 0;
        if (!parseInt(tok[1], 1, MAX_ID, id)) { commandError(lineNo, "invalid incident ID"); return; }
        Incident** p = table.find(id);          // hash table lookup
        if (!p) { commandError(lineNo, "incident " + std::to_string(id) + " not found"); return; }
        printIncident(*p);
        return;
    }
    if (cmd == "INC_LIST") {
        std::cout << "INCIDENTS " << incidents.size() << "\n";
        IncidentLister v;
        incidents.inorder(v);                   // BST in-order
        return;
    }
    if (cmd == "INC_PREORDER") {
        if (incidents.empty()) { std::cout << "INCIDENT PREORDER EMPTY\n"; return; }
        std::cout << "INCIDENT PREORDER";
        IncidentPreorderPrinter v;
        incidents.preorder(v);
        std::cout << "\n";
        return;
    }
    if (cmd == "INC_CLOSE") {
        int id = 0;
        if (!parseInt(tok[1], 1, MAX_ID, id)) { commandError(lineNo, "invalid incident ID"); return; }
        Incident** p = table.find(id);
        if (!p) { commandError(lineNo, "incident " + std::to_string(id) + " not found"); return; }
        Incident* inc = *p;
        if (inc->status == STATUS_PENDING) heap.removeById(id);   // leave the heap first
        closeIncident(inc);
        std::cout << "INCIDENT CLOSED " << id << "\n";
        return;
    }

    // ------------------------------------------------------ priority desk
    if (cmd == "REQUEST") {
        int id = 0;
        if (!parseInt(tok[1], 1, MAX_ID, id)) { commandError(lineNo, "invalid incident ID"); return; }
        Incident** p = table.find(id);
        if (!p) { commandError(lineNo, "incident " + std::to_string(id) + " not found"); return; }
        Incident* inc = *p;
        if (inc->status == STATUS_PENDING) {
            commandError(lineNo, "incident " + std::to_string(id) + " is already PENDING");
            return;
        }
        inc->status = STATUS_PENDING;
        heap.insert(inc);
        std::cout << "INCIDENT PENDING " << id << "\n";
        return;
    }
    if (cmd == "PRIORITY_NEXT") {
        if (heap.empty()) { std::cout << "NO PENDING INCIDENTS\n"; return; }
        printIncident(heap.peek());
        return;
    }
    if (cmd == "PRIORITY_SERVE") {
        if (heap.empty()) { std::cout << "NO PENDING INCIDENTS\n"; return; }
        Incident* inc = heap.extractMax();
        int id = inc->id;
        closeIncident(inc);                     // hash delete + BST delete
        std::cout << "INCIDENT SERVED " << id << "\n";
        return;
    }
    if (cmd == "PRIORITY_COUNT") {
        std::cout << "PENDING " << heap.size() << "\n";
        return;
    }
    if (cmd == "HASH_STATS") {
        std::cout << "HASH capacity=" << table.capacity() << " size=" << table.size() << "\n";
        return;
    }
}
