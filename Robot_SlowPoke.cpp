#include "RobotBase.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <limits>

class Robot_SlowPoke : public RobotBase
{
private:
    int radar_dir = 1;
    bool has_target = false;
    int tr = -1, tc = -1;

    int mdist(int r1,int c1,int r2,int c2) {
        return std::abs(r1-r2)+std::abs(c1-c2);
    }

public:
    Robot_SlowPoke() : RobotBase(2,5,flamethrower)
    {
        std::srand((unsigned)std::time(nullptr));
    }

    virtual void get_radar_direction(int& out) override
    {
        out = radar_dir;
        radar_dir = (radar_dir % 8) + 1;
    }

    virtual void process_radar_results(const std::vector<RadarObj>& r) override
    {
        has_target = false;
        int cr,cc;
        get_current_location(cr,cc);

        for (auto& o : r)
        {
            if (o.m_type == 'R')
            {
                if (!has_target)
                {
                    has_target = true;
                    tr = o.m_row;
                    tc = o.m_col;
                }
            }
        }
    }

    virtual bool get_shot_location(int& sr,int& sc) override
    {
        if (!has_target) return false;

        int cr,cc;
        get_current_location(cr,cc);

        if (mdist(cr,cc,tr,tc) <= 4)
        {
            sr = tr;
            sc = tc;
            return true;
        }
        return false;
    }

    virtual void get_move_direction(int& d,int& dist) override
    {
        d = (std::rand() % 8) + 1;
        dist = 1;
    }
};

extern "C" RobotBase* create_robot() { return new Robot_SlowPoke(); }
