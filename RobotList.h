#pragma once
#include <vector>
#include <memory>
#include <string>
#include "RobotBase.h"

struct RobotEntry {
    std::unique_ptr<RobotBase> instance;
    std::string m_name; // from print_stats or set later
    char glyph = '?'; // character to display, e.g., '@', '$'
    int row = -1;
    int col = -1;
    bool alive = true;

    // Arena-side metadata (since RobotBase cannot be changed)
    std::string lastRadarLog;
    std::string lastShotLog;
    std::string lastMoveLog;

};

class RobotList {
public:
    // Add a robot (takes ownership)
    int add(std::unique_ptr<RobotBase> rb, char glyph) {
        RobotEntry e;
        e.instance = std::move(rb);
        e.glyph = glyph;
        e.alive = true;
        entries.push_back(std::move(e));
        return static_cast<int>(entries.size() - 1);
    }

    size_t size() const { return entries.size(); }

    RobotEntry& operator[](size_t idx) { return entries[idx]; }
    const RobotEntry& operator[](size_t idx) const { return entries[idx]; }

    // Count living robots
    int living_count() const {
        int n = 0;
        for (auto& e : entries) if (e.alive && e.instance->get_health() > 0) ++n;
        return n;
    }

    // Winner index or -1 if none/unfinished
    int find_last_alive() const {
        int idx = -1;
        for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
            const auto& e = entries[i];
            if (e.alive && e.instance->get_health() > 0) {
                if (idx != -1) return -1; // more than one
                idx = i;
            }
        }
        return idx;
    }

private:
    std::vector<RobotEntry> entries;
};
