#include "Arena.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

Arena::Arena(const GameConfig& cfg_in)
    : cfg(cfg_in), board(cfg_in.height, cfg_in.width), rng(cfg_in.rngSeed) {}

char Arena::next_glyph() {
    static const std::string glyphs = "@$#!&%?";
    static size_t i = 0;
    return glyphs[i++ % glyphs.size()];
}

std::pair<int,int> Arena::random_empty_cell() {
    std::uniform_int_distribution<int> rdist(0, cfg.height - 1);
    std::uniform_int_distribution<int> cdist(0, cfg.width - 1);
    for (int tries = 0; tries < 10000; ++tries) {
        int r = rdist(rng), c = cdist(rng);
        if (board.at(r, c).type == '.') return {r, c};
    }
    return {-1, -1};
}

bool Arena::is_cell_free_for_robot(int r, int c) const {
    if (!board.in_bounds(r, c)) return false;
    char t = board.at(r, c).type;
    return t == '.';
}

bool Arena::load_robots_from_sources(const std::string& directory) {
    // Compile and load Robot_*.cpp from directory
    bool anyLoaded = false;
    for (auto& p : std::filesystem::directory_iterator(directory)) {
        if (!p.is_regular_file()) continue;
        auto name = p.path().filename().string();
        if (name.rfind("Robot_", 0) != 0 || p.path().extension() != ".cpp") continue;

        std::string so = "./lib" + p.path().stem().string() + ".so";
        std::string cmd = "g++ -shared -fPIC -o " + so + " " + p.path().string() + " RobotBase.o -I. -std=c++20";
        std::cout << "Compiling " << name << " -> " << so << "\n";
        if (std::system(cmd.c_str()) != 0) {
            std::cerr << "Compile failed: " << name << "\n";
            continue;
        }

        void* handle = dlopen(so.c_str(), RTLD_LAZY);
        if (!handle) { std::cerr << "dlopen failed: " << so << " : " << dlerror() << "\n"; continue; }
        dl_handles.push_back(handle);

        RobotFactory create_robot = (RobotFactory)dlsym(handle, "create_robot");
        if (!create_robot) { std::cerr << "dlsym failed: create_robot in " << so << " : " << dlerror() << "\n"; continue; }

        std::unique_ptr<RobotBase> rb(create_robot());
        if (!rb) { std::cerr << "create_robot returned null for " << so << "\n"; continue; }

        // Set boundaries immediately
        rb->set_boundaries(cfg.height, cfg.width);

        char g = next_glyph();

        // Derive name from filename stem
        std::string stem = p.path().stem().string();   // e.g. "Robot_TuNe"
        if (stem.rfind("Robot_", 0) == 0) {
            stem = stem.substr(6); // strip "Robot_"
        }


        int idx = robots.add(std::move(rb), g, stem);
        std::cout << "Robot added at index " << idx <<  " with name " << stem << "\n";

        anyLoaded = true;
    }
    return anyLoaded;
}

void Arena::place_obstacles() {
    std::uniform_int_distribution<int> rdist(0, cfg.height - 1);
    std::uniform_int_distribution<int> cdist(0, cfg.width - 1);

    auto placeN = [&](int count, char ch) {
        int placed = 0;
        while (placed < count) {
            int r = rdist(rng), c = cdist(rng);
            if (board.at(r, c).type == '.') {
                if (board.place_obstacle(r, c, ch)) ++placed;
            }
        }
    };

    placeN(cfg.pits, 'P');
    placeN(cfg.mounds, 'M');
    placeN(cfg.flamers, 'F');
}

void Arena::place_robots_randomly() {
    for (size_t i = 0; i < robots.size(); ++i) {
        auto& e = robots[i];
        auto [r, c] = random_empty_cell();
        if (r == -1) { std::cerr << "No space to place robot " << i << "\n"; continue; }
        e.row = r; e.col = c; e.alive = true;
        board.place_robot(r, c, static_cast<int>(i), true);
        e.instance->move_to(r, c);
        e.instance->set_boundaries(cfg.height, cfg.width);
    }
}

void Arena::print_round_header(int round) {
    std::cout << "=========== starting round " << round << " ===========" << "\n\n";
}


/*void Arena::print_state() {
    std::cout << board.render() << "\n";

    for (size_t i = 0; i < robots.size(); ++i) {
        auto& e = robots[i];
        RobotBase* r = e.instance.get();

        // Guard against null instance
        if (!r) {
            std::cout << "R" << e.glyph << " (" << e.row << "," << e.col << ") "
                      << "Name: " << e.name << " - instance missing\n";
            continue;
        }

        std::cout << "R" << e.glyph << " (" << e.row << "," << e.col << ") "
                  << "Name: " << e.name << ' '   // use Arena-side name
                  << "Health: " << r->get_health()
                  << " Armor: " << r->get_armor()
                  << (e.alive ? "" : " - is out")
                  << "\n";

        // Optional: print logs if you’ve populated them
        if (e.alive) {
            if (!e.lastRadarLog.empty())
                std::cout << "  " << e.lastRadarLog << "\n";
            if (!e.lastShotLog.empty())
                std::cout << "  " << e.lastShotLog << "\n";
            if (!e.lastMoveLog.empty())
                std::cout << "  " << e.lastMoveLog << "\n";
            std::cout << "\n";
        }
    }

    std::cout << "\n";
}*/

void Arena::print_state() {
    std::cout << board.render() << "\n";

    for (size_t i = 0; i < robots.size(); ++i) {
        auto& e = robots[i];
        RobotBase* r = e.instance.get();

        std::cout << "R" << e.glyph << " (" << e.row << "," << e.col << ") "
                  << "Name: " << e.name << ' ';

        if (r) {
            std::cout << "Health: " << r->get_health()
                      << " Armor: " << r->get_armor();
        } else {
            std::cout << "Health: N/A Armor: N/A";
        }

        if (!e.alive) {
            std::cout << " - is out";
        }
        std::cout << "\n";

        if (e.alive && r) {
            if (!e.lastRadarLog.empty()) std::cout << "  " << e.lastRadarLog << "\n";
            if (!e.lastShotLog.empty())  std::cout << "  " << e.lastShotLog << "\n";
            if (!e.lastMoveLog.empty())  std::cout << "  " << e.lastMoveLog << "\n";
            std::cout << "\n";
        }
    }

    std::cout << "\n";
}



bool Arena::check_winner(int& winnerIdx) {
    winnerIdx = robots.find_last_alive();
    return winnerIdx != -1;
}

std::vector<RadarObj> Arena::perform_radar(int robotIdx, int radarDirection) {
    std::vector<RadarObj> out;
    auto& e = robots[robotIdx];
    int r0 = e.row, c0 = e.col;

    if (radarDirection == 0) {
        // 8 neighbors
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int r = r0 + dr, c = c0 + dc;
                if (!board.in_bounds(r, c)) continue;
                out.emplace_back(board.at(r, c).type, r, c);
            }
        }
        return out;
    }

    // Directions 1..8 with 3-wide ray (perpendicular offsets -1..+1)
    auto d = directions[radarDirection];
    int dr = d.first, dc = d.second;
    int pr = -dc, pc = dr; // perpendicular vector

    // Walk to edge
    int r = r0 + dr, c = c0 + dc;
    while (board.in_bounds(r, c)) {
        for (int w = -1; w <= 1; ++w) {
            int rw = r + pr * w;
            int cw = c + pc * w;
            if (!board.in_bounds(rw, cw)) continue;
            if (rw == r0 && cw == c0) continue;
            out.emplace_back(board.at(rw, cw).type, rw, cw);
        }
        r += dr; c += dc;
    }
    return out;
}

void Arena::handle_shot(int shooterIdx, int shotRow, int shotCol) {
    // Validate shooter index
    if (shooterIdx < 0 || shooterIdx >= static_cast<int>(robots.size())) return;

    RobotEntry& shooterEntry = robots[shooterIdx];
    RobotBase* shooter = shooterEntry.instance.get();

    // Shooter must be alive and have a valid instance
    if (!shooterEntry.alive || shooter == nullptr) return;

    // Ensure shot coordinates are in bounds before any logic that relies on them
    if (!board.in_bounds(shotRow, shotCol)) {
        std::cout << "Robot " << shooterEntry.glyph
                  << " attempted an out-of-bounds shot at (" << shotRow << "," << shotCol << ")\n";
        return;
    }

    std::cout << "Robot " << shooterEntry.glyph
              << " fired a shot at (" << shotRow << "," << shotCol << ")\n";

    // Weapon query (safe: shooter != nullptr above)
    WeaponType w = shooter->get_weapon();

    for (size_t i = 0; i < robots.size(); ++i) {
        RobotEntry& e = robots[i];
        RobotBase* target = e.instance.get();

        // Skip dead or missing instances
        if (!e.alive || target == nullptr) continue;

        bool hit = false;

        switch (w) {
            case WeaponType::railgun:
                // Line attack: same row or col
                hit = (e.row == shotRow || e.col == shotCol);
                break;

            case WeaponType::hammer:
            case WeaponType::grenade:
            case WeaponType::flamethrower:
                // 3x3 AoE centered on shot
                hit = (std::abs(e.row - shotRow) <= 1 && std::abs(e.col - shotCol) <= 1);
                break;

            default:
                // Fist or unknown: direct cell only
                hit = (e.row == shotRow && e.col == shotCol);
                break;
        }

        // Optional: prevent self-hit if your design requires it
        // if (static_cast<int>(i) == shooterIdx) hit = false;

        if (!hit) continue;

        std::cout << "Robot " << shooterEntry.glyph
                  << " hit Robot " << e.glyph
                  << " at (" << e.row << "," << e.col << ")\n";

        // Null-safe stat reads and calculations
        int armor = target->get_armor();           // target != nullptr ensured
        int raw = 10;                              // placeholder damage
        double reduction = 0.1 * static_cast<double>(armor);
        int dealt = std::max(0, static_cast<int>(std::round(raw * (1.0 - reduction))));

        // Apply damage safely
        target->take_damage(dealt);
        target->reduce_armor(1);

        // Recheck health via the valid pointer
        int health = target->get_health();

        if (health <= 0) {
            e.alive = false;

            // Only touch the board if coordinates are valid
            if (board.in_bounds(e.row, e.col)) {
                board.set_dead(e.row, e.col);
            } else {
                std::cout << "Warning: dead robot " << e.glyph
                          << " has out-of-bounds coords (" << e.row << "," << e.col
                          << "); skipping board update.\n";
            }

            std::cout << "Robot " << e.glyph << " has been destroyed!\n";

            // Optional: do NOT reset e.instance here if you still need to print stats later.
            // e.instance.reset(); // If you choose to free immediately, ensure all later code is null-safe.
        }
    }
}

/*void Arena::handle_shot(int shooterIdx, int shotRow, int shotCol) {
    if (shooterIdx < 0 || shooterIdx >= (int)robots.size()) return;
    if (!robots[shooterIdx].alive || robots[shooterIdx].instance == nullptr) return;

    std::cout << "Robot " << robots[shooterIdx].glyph
              << " fired a shot at (" << shotRow << "," << shotCol << ")\n";

    WeaponType w = robots[shooterIdx].instance->get_weapon();

    for (size_t i = 0; i < robots.size(); ++i) {
        auto& e = robots[i];
        if (!e.alive || e.instance == nullptr) continue;

        bool hit = false;

        switch (w) {
            case WeaponType::railgun:
                // Example: hit if same row or same col along path
                if (e.row == shotRow || e.col == shotCol) hit = true;
                break;

            case WeaponType::hammer:
                // Adjacent only
                if (std::abs(e.row - shotRow) <= 1 && std::abs(e.col - shotCol) <= 1)
                    hit = true;
                break;

            case WeaponType::grenade:
                // AoE radius 1 around shot cell
                if (std::abs(e.row - shotRow) <= 1 && std::abs(e.col - shotCol) <= 1)
                    hit = true;
                break;

            case WeaponType::flamethrower:
                // Simple stub: 3×3 area centered on shot
                if (std::abs(e.row - shotRow) <= 1 && std::abs(e.col - shotCol) <= 1)
                    hit = true;
                break;

            default:
                // Fist or unknown: direct cell only
                if (e.row == shotRow && e.col == shotCol) hit = true;
                break;
        }

        if (hit) {
            std::cout << "Robot " << robots[shooterIdx].glyph
                      << " hit Robot " << e.glyph
                      << " at (" << e.row << "," << e.col << ")\n";

            int raw = 10; // placeholder damage
            double reduction = 0.1 * e.instance->get_armor();
            int dealt = std::max(0, (int)std::round(raw * (1.0 - reduction)));

            e.instance->take_damage(dealt);
            e.instance->reduce_armor(1);

            if (e.instance->get_health() <= 0) {
                e.alive = false;
                board.set_dead(e.row, e.col);
                std::cout << "Robot " << e.glyph << " has been destroyed!\n";
            }
        }
    }
}*/

void Arena::handle_move(int robotIdx, int moveDir, int distance) {
    auto& e = robots[robotIdx];
    if (!e.alive) return;

    // Cap by robot max move
    int maxMove = e.instance->get_move_speed();
    if (distance > maxMove) distance = maxMove;

    auto d = directions[moveDir];
    int dr = d.first, dc = d.second;

    int r = e.row, c = e.col;
    for (int step = 0; step < distance; ++step) {
        int nr = r + dr, nc = c + dc;
        if (!board.in_bounds(nr, nc)) break;

        char t = board.at(nr, nc).type;
        if (t == 'M' || t == 'R' || t == 'X') {
            // stop before obstacle/robot
            break;
        } else if (t == 'P') {
            // move onto pit and trap
            board.vacate(r, c);
            board.place_robot(nr, nc, robotIdx, true);
            e.row = nr; e.col = nc;
            e.instance->move_to(nr, nc);
            e.instance->disable_movement(); // trapped
            break;
        } else if (t == 'F') {
            // move through and take flamethrower damage (placeholder range 30–50)
            board.vacate(r, c);
            board.place_robot(nr, nc, robotIdx, true);
            r = nr; c = nc;
            e.row = r; e.col = c;
            e.instance->move_to(r, c);
            int raw = 40; // placeholder mid‑range
            double reduction = 0.1 * e.instance->get_armor();
            int dealt = std::max(0, (int)std::round(raw * (1.0 - reduction)));
            e.instance->take_damage(dealt);
            e.instance->reduce_armor(1);
            if (e.instance->get_health() <= 0) {
                e.alive = false;
                board.set_dead(e.row, e.col);
                break;
            }
        } else {
            // empty
            board.vacate(r, c);
            board.place_robot(nr, nc, robotIdx, true);
            r = nr; c = nc;
            e.row = r; e.col = c;
            e.instance->move_to(r, c);
        }
    }
}

void Arena::run() {
    place_obstacles();
    place_robots_randomly();

    int winner = -1;
    for (int round = 1; round <= cfg.maxRounds; ++round) {
        print_round_header(round);
        print_state();

        if (check_winner(winner)) {
            std::cout << "Winner: R" << robots[winner].glyph
                      << " at (" << robots[winner].row << "," << robots[winner].col << ")"
                      << " Name: " << robots[winner].name << "\n";
            break;
        }

        for (size_t i = 0; i < robots.size(); ++i) {
            auto& e = robots[i];
            if (!e.alive || e.instance == nullptr) continue;

            int radarDir = 0;
            e.instance->get_radar_direction(radarDir);
            auto scan = perform_radar(static_cast<int>(i), radarDir);
            e.instance->process_radar_results(scan);

            int shotRow = 0, shotCol = 0;
            if (e.instance->get_shot_location(shotRow, shotCol)) {
                handle_shot(static_cast<int>(i), shotRow, shotCol);
            } else {
                int moveDir = 0, steps = 0;
                e.instance->get_move_direction(moveDir, steps);
                if (moveDir != 0 && steps > 0) {
                    handle_move(static_cast<int>(i), moveDir, steps);
                }
            }

            if (cfg.liveView) {
                print_state();
                std::this_thread::sleep_for(std::chrono::milliseconds(600));
            }
        }
    }

    // If no winner after all rounds → draw
    if (winner == -1) {
        std::cout << "The battle ended in a draw after "
                  << cfg.maxRounds << " rounds.\n";
        std::cout << "Robots still standing:\n";
        for (size_t i = 0; i < robots.size(); ++i) {
            const auto& e = robots[i];
            if (e.alive && e.instance != nullptr) {
                std::cout << "  R" << e.glyph
                          << " (" << e.row << "," << e.col << ") "
                          << "Name: " << e.name
                          << " Health: " << e.instance->get_health()
                          << " Armor: " << e.instance->get_armor()
                          << "\n";
            }
        }
    }

    // Cleanup loaded shared libs
    for (size_t i = 0; i < robots.size(); ++i) {
        robots[i].instance.reset();
    }
    for (void* h : dl_handles) {
        if (h) dlclose(h);
    }
}

/*void Arena::run() {
    place_obstacles();
    place_robots_randomly();

    int winner = -1;
    for (int round = 1; round <= cfg.maxRounds; ++round) {
        print_round_header(round);
        print_state();

        if (check_winner(winner)) {
            std::cout << "Winner: R" << robots[winner].glyph
                      << " at (" << robots[winner].row << "," << robots[winner].col << ")\n";
                      
            break;
        }

        for (size_t i = 0; i < robots.size(); ++i) {
            auto& e = robots[i];
            //if (!e.alive) continue;
            if (!e.alive || e.instance == nullptr) continue;


            int radarDir = 0;
            e.instance->get_radar_direction(radarDir);
            auto scan = perform_radar(static_cast<int>(i), radarDir);
            e.instance->process_radar_results(scan);

            int shotRow = 0, shotCol = 0;
            if (e.instance->get_shot_location(shotRow, shotCol)) {
                handle_shot(static_cast<int>(i), shotRow, shotCol);
            } else {
                int moveDir = 0, steps = 0;
                e.instance->get_move_direction(moveDir, steps);
                if (moveDir != 0 && steps > 0) {
                    handle_move(static_cast<int>(i), moveDir, steps);
                }
            }

            if (cfg.liveView) {
                print_state();
                std::this_thread::sleep_for(std::chrono::milliseconds(600));
            }
        }
    }


    // Cleanup loaded shared libs
    // First destroy robot instances
    for (size_t i = 0; i < robots.size(); ++i) {
        robots[i].instance.reset();
    }

    // Then close dynamic libraries
    for (void* h : dl_handles) {
        if (h) dlclose(h);
    }
}*/
