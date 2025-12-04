#include "RobotBase.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

class Robot_TuNe_pro : public RobotBase {
public:
    Robot_TuNe_pro() : RobotBase(2, 3, WeaponType::grenade) {
        m_name = "T_pro";
        //glyph = '$';
        grenades_left = 15;
        current_corner = 0;
        zigzag_toggle = false;
    }

    void get_radar_direction(int& radar_direction) override {
        // Directional radar: scan forward + sides based on last move
        // 1=N, 2=E, 3=S, 4=W
        switch (last_dir) {
            case 1: radar_direction = 1; break; // north
            case 2: radar_direction = 2; break; // east
            case 3: radar_direction = 3; break; // south
            case 4: radar_direction = 4; break; // west
            default: radar_direction = 0; break; // fallback: all around
        }
    }

    void process_radar_results(const std::vector<RadarObj>& results) override {
        has_adjacent_target = false;
        target_row = -1;
        target_col = -1;
        target_health = 999;
        target_armor = 999;

        for (const auto& obj : results) {
            if (obj.m_type == 'R') {
                // Adjacent robot found
                target_row = obj.m_row;
                target_col = obj.m_col;
                has_adjacent_target = true;
                // For smarter grenade targeting, store dummy health/armor
                // (Arena doesn’t give enemy stats directly, so assume low)
                target_health = 40; 
                target_armor = 1;
                break;
            }
        }
    }

    bool get_shot_location(int& row, int& col) override {
        if (has_adjacent_target && grenades_left > 0) {
            // Grenade efficiency: only throw at weak/low armor targets
            if (target_armor <= 1 || target_health < 50) {
                row = target_row;
                col = target_col;
                grenades_left--;
                return true;
            }
        }
        // No grenades or not worth it → don’t attack
        return false;
    }

    void get_move_direction(int& dir, int& dist) override {
        if (has_adjacent_target && grenades_left > 0) {
            // Stay put to attack
            dir = 0;
            dist = 0;
            return;
        }

        // If grenades are gone or armor is low → run away
        if (grenades_left == 0 || get_armor() <= 1) {
            // Zig‑zag retreat pattern
            zigzag_toggle = !zigzag_toggle;
            dir = zigzag_toggle ? 1 : 2; // alternate N/E
            dist = 1;
            last_dir = dir;
            return;
        }

        // Smarter corner choice: pick farthest corner from target
        std::pair<int,int> corners[4] = {{0,0},{0,19},{19,0},{19,19}};
        int bestDist = -1;
        int bestCorner = 0;
        for (int i=0; i<4; i++) {
            int distToCorner = 0;
            if (target_row >= 0 && target_col >= 0) {
                distToCorner = std::abs(corners[i].first - target_row) +
                               std::abs(corners[i].second - target_col);
            }
            if (distToCorner > bestDist) {
                bestDist = distToCorner;
                bestCorner = i;
            }
        }

        // Move toward chosen corner
        switch (bestCorner) {
            case 0: dir = 1; dist = 2; break; // NW → north
            case 1: dir = 2; dist = 2; break; // NE → east
            case 2: dir = 4; dist = 2; break; // SW → west
            case 3: dir = 3; dist = 2; break; // SE → south
        }
        last_dir = dir;
    }

private:
    int grenades_left;
    int current_corner;
    bool has_adjacent_target = false;
    int target_row, target_col;
    int target_health, target_armor;

    int last_dir = 0;
    bool zigzag_toggle;
};

// Factory function
extern "C" RobotBase* create_robot() {
    return new Robot_TuNe_pro();
}
