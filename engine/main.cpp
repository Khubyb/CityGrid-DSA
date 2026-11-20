#include <iostream>
#include <fstream>
#include <string>

#include "CityGrid.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "USAGE: citygrid <junctions file> <incidents file> <script file>\n";
        return 1;
    }

    // Open the files in order: junctions, incidents, script.
    std::ifstream junctionsIn(argv[1]);
    if (!junctionsIn.is_open()) {
        std::cout << "FATAL: cannot open " << argv[1] << "\n";
        return 1;
    }
    std::ifstream incidentsIn(argv[2]);
    if (!incidentsIn.is_open()) {
        std::cout << "FATAL: cannot open " << argv[2] << "\n";
        return 1;
    }
    std::ifstream scriptIn(argv[3]);
    if (!scriptIn.is_open()) {
        std::cout << "FATAL: cannot open " << argv[3] << "\n";
        return 1;
    }

    {
        CityGrid grid;   // destructor frees everything when this scope ends
        grid.loadJunctions(junctionsIn, argv[1]);
        grid.loadIncidents(incidentsIn, argv[2]);
        grid.runScript(scriptIn);
    }
    return 0;
}
