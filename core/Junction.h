#ifndef JUNCTION_H
#define JUNCTION_H

#include <string>

// A junction (road intersection or facility). Stored BY VALUE inside the AVL tree.
struct Junction {
    int id;
    std::string name;
    std::string type;   // HOSPITAL, FIRE, POLICE, DEPOT or PLAIN

    Junction() : id(0), name(""), type("") {}
    Junction(int i, const std::string& n, const std::string& t) : id(i), name(n), type(t) {}
};

#endif
