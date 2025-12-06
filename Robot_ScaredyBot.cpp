#include "RobotBase.h"
#include <cstdlib>
#include <ctime>
#include <cmath>

class Robot_ScaredyBot : public RobotBase
{
private:
    bool has_target=false;
    int tr=-1, tc=-1;

    int mdist(int r1,int c1,int r2,int c2)
    { return std::abs(r1-r2)+std::abs(c1-c2); }

public:
    Robot_ScaredyBot() : RobotBase(2,5,flamethrower)
    {
        std::srand((unsigned)std::time(nullptr));
    }

    virtual void get_radar_direction(int& out) override
    {
        out = (std::rand() % 8) + 1;
    }

    virtual void process_radar_results(const std::vector<RadarObj>& r) override
    {
        has_target = false;
        for (auto& o : r)
        {
            if (o.m_type=='R')
            {
                has_target = true;
                tr=o.m_row;
                tc=o.m_col;
            }
        }
    }

    virtual bool get_shot_location(int& sr,int& sc) override
    {
        return false; // Coward, never shoots
    }

    virtual void get_move_direction(int& d,int& dist) override
    {
        int cr,cc;
        get_current_location(cr,cc);

        if (!has_target)
        {
            d = (std::rand()%8)+1;
            dist=1;
            return;
        }

        // Move away from target
        int dr = tr - cr;
        int dc = tc - cc;

        if (dr < 0 && dc == 0) d = 5; // enemy is up → go down
        else if (dr > 0 && dc == 0) d = 1; // enemy down → go up
        else if (dr == 0 && dc < 0) d = 3; // enemy left → go right
        else d = 7; // enemy right → go left

        dist = 1;
    }
};

extern "C" RobotBase* create_robot() { return new Robot_ScaredyBot(); }
