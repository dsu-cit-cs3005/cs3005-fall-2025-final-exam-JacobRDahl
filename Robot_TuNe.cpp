#include "RobotBase.h"
#include <vector>
#include <string>

class Robot_TuNe : public RobotBase {
public:
    Robot_TuNe() : RobotBase(2, 3, WeaponType::grenade) {
        m_name = "TuNe";
        //glyph = '&';
        grenades_left = 15;   // starting grenade count
        current_corner = 0;  // start at corner 0
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
        if (has_adjacent_target && grenades_left > 0) {
            // Attack only if grenades remain
            row = target_row;
            col = target_col;
            grenades_left--;
            return true;
        }
        // If grenades are gone, do not attack — just run away
        return false;
    }

    void get_move_direction(int& dir, int& dist) override {
        if (has_adjacent_target && grenades_left > 0) {
            // Stay put to attack
            dir = 0;
            dist = 0;
            return;
        }
    }
   

private:
    int grenades_left;
    int current_corner;
    bool has_adjacent_target = false;
    int target_row = 0;
    int target_col = 0;
    
};

// Factory function
extern "C" RobotBase* create_robot() {
    return new Robot_TuNe();
}
