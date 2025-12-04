#pragma once
#include <string>
#include <vector>
#include <random>
#include <filesystem>
#include <dlfcn.h>
#include <unordered_set>

#include "PlayingBoard.h"
#include "RobotList.h"
#include "RadarObj.h"
#include "RobotBase.h"

struct GameConfig {
    int width = 20;
    int height = 20;
    int mounds = 5;
    int pits = 3;
    int flamers = 3;
    int maxRounds = 100;
    bool liveView = true;
    unsigned rngSeed = 42;
};

class Arena {
public:
    Arena(const GameConfig& cfg);

    bool load_robots_from_sources(const std::string& directory);
    void place_obstacles();
    void place_robots_randomly();

    void run();

private:
    GameConfig cfg;
    PlayingBoard board;
    RobotList robots;

    std::mt19937 rng;
    std::vector<void*> dl_handles;

    // helpers
    std::pair<int,int> random_empty_cell();
    char next_glyph();

    void print_round_header(int round);
    void print_state();
    bool check_winner(int& winnerIdx);

    // action orchestration stubs (to be expanded with full rules)
    std::vector<RadarObj> perform_radar(int robotIdx, int radarDirection);
    void handle_shot(int shooterIdx, int shotRow, int shotCol);
    void handle_move(int robotIdx, int moveDir, int distance);

    // placement safety
    bool is_cell_free_for_robot(int r, int c) const;
};
