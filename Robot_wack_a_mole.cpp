#include "RobotBase.h"
#include <vector>
#include <string>
#include <cmath>

class Robot_wack_a_mole : public RobotBase {
public:
    Robot_wack_a_mole() : RobotBase(2, 2, WeaponType::hammer) {
        m_name = "Wack_a_mole";
        //glyph = '#';
        phase = 0;        // 0 = heading to corner, 1 = spiral
        spiral_step = 1;  // current step length
        spiral_dir = 1;   // start moving north
        spiral_count = 0; // steps taken in current direction
        spiral_turns = 0; // number of turns taken
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = 0; // scan all adjacent cells
    }

    void process_radar_results(const std::vector<RadarObj>& results) override {
        has_adjacent_target = false;
        for (const auto& obj : results) {
            if (obj.m_type == 'R') {
                target_row = obj.m_row;
                target_col = obj.m_col;
                has_adjacent_target = true;
                break;
            }
        }
    }

    bool get_shot_location(int& row, int& col) override {
    if (has_adjacent_target) {
        // Hammer only works if target is adjacent
        row = target_row;
        col = target_col;
        return true;
    }
    return false;
}

    void get_move_direction(int& dir, int& dist) override {
        if (has_adjacent_target) {
            dir = 0; dist = 0; // stay put to swing hammer
            return;
        }

        if (phase == 0) {
            // Head to a corner first (e.g., top-left)
            dir = 1; // north
            dist = 3;
            phase = 1; // once moved, switch to spiral
        } else {
            // Spiral movement outward
            dir = spiral_dir;
            dist = 1; // move one step at a time

            spiral_count++;
            if (spiral_count >= spiral_step) {
                // turn clockwise
                spiral_dir = (spiral_dir % 4) + 1;
                spiral_count = 0;
                spiral_turns++;
                if (spiral_turns % 2 == 0) {
                    // every two turns, increase step length
                    spiral_step++;
                }
            }
        }
    }

private:
    bool has_adjacent_target = false;
    int target_row = 0;
    int target_col = 0;

    int phase;          // 0 = corner, 1 = spiral
    int spiral_step;    // current step length
    int spiral_dir;     // current direction (1=N,2=E,3=S,4=W)
    int spiral_count;   // steps taken in current direction
    int spiral_turns;   // number of turns taken
};

// Factory function
extern "C" RobotBase* create_robot() {
    return new Robot_wack_a_mole();
}