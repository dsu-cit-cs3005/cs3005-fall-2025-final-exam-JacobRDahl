#include "RobotBase.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <unordered_map>

struct EnemyTrack {
    int row = -1;
    int col = -1;
    int prev_row = -1;
    int prev_col = -1;
    int seen_ticks = 0;
    int last_seen_turn = -1;
};

class Robot_Oracle : public RobotBase {
public:
    Robot_Oracle() : RobotBase(2, 2, WeaponType::railgun) {
        m_name = "Orc";
        //glyph = '*';
        last_dir = 2;        // start facing East
        turn_counter = 0;
        mode = Mode::Search;
        sweep_length = 1;
        sweep_count = 0;
        sweep_turns = 0;
        sweep_dir = 1;       // start North
        kiting_toggle = false;
    }

    void get_radar_direction(int& radar_direction) override {
        if (mode == Mode::Engage || mode == Mode::Pressure) {
            radar_direction = last_dir;
            return;
        }
        if ((turn_counter % 4) == 0) {
            radar_direction = 0; // broad scan
        } else {
            radar_direction = last_dir;
        }
    }

    void process_radar_results(const std::vector<RadarObj>& results) override {
        turn_counter++;

        // Update enemy tracks
        for (const auto& obj : results) {
            if (obj.m_type != 'R') continue;

            // Use row/col as pseudo‑unique key
            int key = obj.m_row * 100 + obj.m_col;
            auto& tr = tracks[key];

            tr.prev_row = tr.row;
            tr.prev_col = tr.col;
            tr.row = obj.m_row;
            tr.col = obj.m_col;
            tr.seen_ticks++;
            tr.last_seen_turn = turn_counter;
        }

        // Prune stale tracks
        for (auto it = tracks.begin(); it != tracks.end(); ) {
            if (turn_counter - it->second.last_seen_turn > 6) {
                it = tracks.erase(it);
            } else {
                ++it;
            }
        }

        // Pick best target
        has_target = false;
        predicted_row = -1;
        predicted_col = -1;

        int best_score = -100000;
        for (const auto& kv : tracks) {
            const auto& tr = kv.second;
            if (tr.row < 0 || tr.col < 0) continue;

            int drow = (tr.prev_row >= 0) ? (tr.row - tr.prev_row) : 0;
            int dcol = (tr.prev_col >= 0) ? (tr.col - tr.prev_col) : 0;

            int prow = tr.row + drow;
            int pcol = tr.col + dcol;

            int stability = (std::abs(drow) + std::abs(dcol) == 0) ? 2 : 1;
            int recency = std::max(0, 6 - (turn_counter - tr.last_seen_turn));
            int score = 20 * stability + 15 * recency;

            if (drow == 0 || dcol == 0) score += 10;

            if (score > best_score) {
                best_score = score;
                predicted_row = std::clamp(prow, 0, 19);
                predicted_col = std::clamp(pcol, 0, 19);
                has_target = true;
            }
        }

        if (has_target) {
            mode = Mode::Engage;
        } else {
            bool had_recent = false;
            for (const auto& kv : tracks) {
                if (turn_counter - kv.second.last_seen_turn <= 3) {
                    had_recent = true; break;
                }
            }
            mode = had_recent ? Mode::Pressure : Mode::Search;
        }
    }

    bool get_shot_location(int& row, int& col) override {
        if (!has_target) return false;
        row = predicted_row;
        col = predicted_col;
        return true;
    }

    void get_move_direction(int& dir, int& dist) override {
        if (get_armor() <= 1) {
            kiting_toggle = !kiting_toggle;
            dir = kiting_toggle ? 1 : 2; // N/E dodge
            dist = 1;
            last_dir = dir;
            return;
        }

        switch (mode) {
            case Mode::Engage: {
                static bool horizontal = true;
                dir = horizontal ? 2 : 1; // east/north alternation
                horizontal = !horizontal;
                dist = 1;
                last_dir = dir;
                break;
            }
            case Mode::Pressure: {
                dir = ((turn_counter / 3) % 4) + 1; // rotate N,E,S,W
                dist = 1;
                last_dir = dir;
                break;
            }
            case Mode::Search:
            default: {
                dir = sweep_dir;
                dist = 1;
                sweep_count++;
                if (sweep_count >= sweep_length) {
                    sweep_dir = (sweep_dir % 4) + 1;
                    sweep_count = 0;
                    sweep_turns++;
                    if (sweep_turns % 2 == 0) {
                        sweep_length++;
                    }
                }
                last_dir = dir;
                break;
            }
        }
    }

private:
    enum class Mode { Search, Pressure, Engage };

    Mode mode;
    int last_dir;
    int turn_counter;

    std::unordered_map<int, EnemyTrack> tracks;

    bool has_target = false;
    int predicted_row = -1;
    int predicted_col = -1;

    int sweep_length;
    int sweep_count;
    int sweep_turns;
    int sweep_dir;

    bool kiting_toggle;
};

// Factory function
extern "C" RobotBase* create_robot() {
    return new Robot_Oracle();
}
