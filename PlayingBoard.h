#pragma once
#include <vector>
#include <string>
#include <optional>

// Board cell types encoded as chars, matching RadarObj conventions
// 'X' dead robot, 'R' live robot, 'M' mound, 'F' flamethrower, 'P' pit, '.' empty
struct BoardCell {
    char type = '.';
    // If robot-occupied, index in robot list; otherwise -1
    int robotIndex = -1;
    std::string display;
};

class PlayingBoard {
public:
    PlayingBoard(int rows, int cols)
        : m_rows(rows), m_cols(cols), m_grid(rows, std::vector<BoardCell>(cols)) {}

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }

    bool in_bounds(int r, int c) const { return r >= 0 && r < m_rows && c >= 0 && c < m_cols; }

    BoardCell& at(int r, int c) { return m_grid[r][c]; }
    const BoardCell& at(int r, int c) const { return m_grid[r][c]; }

    void clear() {
        for (auto& row : m_grid) {
            for (auto& cell : row) {
                cell.type = '.';
                cell.robotIndex = -1;
            }
        }
    }

    // Simple placement helpers
    bool place_obstacle(int r, int c, char kind) {
        if (!in_bounds(r, c)) return false;
        if (kind != 'M' && kind != 'F' && kind != 'P') return false;
        auto& cell = at(r, c);
        if (cell.type == 'R' || cell.type == 'X') return false; // no obstacle on robots
        cell.type = kind;
        cell.robotIndex = -1;
        return true;
    }

    bool place_robot(int r, int c, int robotIndex, bool alive = true) {
        if (!in_bounds(r, c)) return false;
        
        auto& cell = at(r, c);
        if (cell.type != '.') return false; // only empty
        cell.type = alive ? 'R' : 'X';
        cell.robotIndex = robotIndex;
        return true;
    }

    void set_dead(int r, int c) {
        if (!in_bounds(r, c)) return;
        auto& cell = at(r, c);
        if (cell.type == 'R') cell.type = 'X';
    }

    void vacate(int r, int c) {
        if (!in_bounds(r, c)) return;
        auto& cell = at(r, c);
        cell.type = '.';
        cell.robotIndex = -1;
    }

    std::string render() const {
        std::string out;
        // header
        out += "    ";
        for (int c = 0; c < m_cols; ++c) {
            out += (c < 10 ? " " : "") + std::to_string(c) + " ";
        }
        out += "\n";
        for (int r = 0; r < m_rows; ++r) {
            out += (r < 10 ? " " : "") + std::to_string(r) + "  ";

            for (int c = 0; c < m_cols; ++c) {
                out += m_grid[r][c].type;
                out += "  ";
            }
            
            out += "\n";
        }
        return out;
    }

private:
    int m_rows;
    int m_cols;
    std::vector<std::vector<BoardCell>> m_grid;
};
