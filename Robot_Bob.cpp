#include "RobotBase.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <limits>

class Robot_Bob : public RobotBase
{
private:
    bool has_target = false;
    int target_row = -1;
    int target_col = -1;

    const int weapon_range = 4; // flamethrower range

    // Manhattan distance
    int dist(int r1, int c1, int r2, int c2) const
    {
        return std::abs(r1 - r2) + std::abs(c1 - c2);
    }

public:
    Robot_Bob() : RobotBase(2, 5, flamethrower)
    {
        std::srand((unsigned)std::time(nullptr));
    }

    // Bob always points radar in the direction of the last known enemy
    virtual void get_radar_direction(int& out_dir) override
    {
        if (!has_target)
        {
            // Scan all directions very quickly (1–8)
            out_dir = (std::rand() % 8) + 1;
            return;
        }

        // Convert target vector to one of the 8 directions
        int r, c;
        get_current_location(r, c);

        int dr = target_row - r;
        int dc = target_col - c;

        // Map vector to direction index
        if (dr < 0 && dc == 0) out_dir = 1;       // up
        else if (dr < 0 && dc > 0) out_dir = 2;  // up-right
        else if (dr == 0 && dc > 0) out_dir = 3; // right
        else if (dr > 0 && dc > 0) out_dir = 4;  // down-right
        else if (dr > 0 && dc == 0) out_dir = 5; // down
        else if (dr > 0 && dc < 0) out_dir = 6;  // down-left
        else if (dr == 0 && dc < 0) out_dir = 7; // left
        else out_dir = 8;                       // up-left
    }

    // Pick the closest enemy immediately
    virtual void process_radar_results(const std::vector<RadarObj>& radar) override
    {
        has_target = false;
        int r, c;
        get_current_location(r, c);

        int best_dist = std::numeric_limits<int>::max();

        for (const auto& o : radar)
        {
            if (o.m_type == 'R') // enemy robot
            {
                int d = dist(r, c, o.m_row, o.m_col);

                if (d < best_dist)
                {
                    best_dist = d;
                    target_row = o.m_row;
                    target_col = o.m_col;
                    has_target = true;
                }
            }
        }
    }

    // Shoot as soon as enemy is in range
    virtual bool get_shot_location(int& sr, int& sc) override
    {
        if (!has_target) return false;

        int r, c;
        get_current_location(r, c);

        if (dist(r, c, target_row, target_col) <= weapon_range)
        {
            sr = target_row;
            sc = target_col;
            return true;
        }

        return false;
    }

    // Bob aggressively moves toward the enemy
    virtual void get_move_direction(int& dir, int& dist_out) override
    {
        int r, c;
        get_current_location(r, c);

        if (!has_target)
        {
            // If lost target, rotate/search
            dir = (std::rand() % 8) + 1;
            dist_out = 1;
            return;
        }

        int dr = target_row - r;
        int dc = target_col - c;

        // Move 1 step toward the target
        if (dr < 0 && dc == 0) dir = 1;
        else if (dr < 0 && dc > 0) dir = 2;
        else if (dr == 0 && dc > 0) dir = 3;
        else if (dr > 0 && dc > 0) dir = 4;
        else if (dr > 0 && dc == 0) dir = 5;
        else if (dr > 0 && dc < 0) dir = 6;
        else if (dr == 0 && dc < 0) dir = 7;
        else dir = 8;

        dist_out = 1;
    }
};

extern "C" RobotBase* create_robot()
{
    return new Robot_Bob();
}
