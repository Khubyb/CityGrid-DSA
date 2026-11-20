#ifndef CITYGRID_H
#define CITYGRID_H

#include <string>
#include <istream>

#include "Junction.h"
#include "Incident.h"
#include "AVL.h"
#include "BST.h"
#include "HashMap.h"
#include "MaxHeap.h"

// CityGrid owns the four structures and keeps them synchronized:
//   AVL     : junction directory
//   BST     : incident registry (OWNS every Incident object)
//   HashMap : fast incident lookup (same Incident* pointers)
//   MaxHeap : pending emergency queue (same Incident* pointers)
// All parsing / validation / printing lives here, never in the structures.
class CityGrid {
public:
    CityGrid();
    ~CityGrid();

    void loadJunctions(std::istream& in, const std::string& fileName);
    void loadIncidents(std::istream& in, const std::string& fileName);
    void runScript(std::istream& in);

private:
    AVL<int, Junction>     junctions;
    BST<int, Incident*>    incidents;
    HashMap<int, Incident*> table;
    MaxHeap<Incident*>     heap;

    CityGrid(const CityGrid&);              // no copying
    CityGrid& operator=(const CityGrid&);

    void execute(const std::string& trimmedLine, int lineNo);
    void commandError(int lineNo, const std::string& reason);

    // helpers shared by file loading and INC_REPORT
    void registerIncident(int id, int junctionId, const std::string& category,
                          int severity, int timestamp);
    void closeIncident(Incident* inc);      // removes from hash + BST (frees object)

    static void printJunction(const Junction& j);
    static void printIncident(const Incident* inc);
};

#endif
