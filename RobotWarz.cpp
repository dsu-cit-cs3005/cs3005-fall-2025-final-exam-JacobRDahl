#include <iostream>
#include "Arena.h"

int main(int argc, char** argv) {
    // Optional CLI: arena size and robots directory
    std::string robotsDir = ".";
    if (argc >= 2) robotsDir = argv[1];

    GameConfig cfg;
    cfg.width = 20;
    cfg.height = 20;
    cfg.mounds = 5;
    cfg.pits = 3;
    cfg.flamers = 3;
    cfg.maxRounds = 50;
    cfg.liveView = true;
    cfg.rngSeed = 1234;

    Arena arena(cfg);

    if (!arena.load_robots_from_sources(robotsDir)) {
        std::cerr << "No robots loaded from: " << robotsDir << "\n";
        return 1;
    }

    arena.run();
    return 0;
}
