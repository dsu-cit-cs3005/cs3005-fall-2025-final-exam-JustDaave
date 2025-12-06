#include "RobotBase.h"
#include <cstdlib>
#include <ctime>

class Robot_BlindBot : public RobotBase
{
private:
    bool has_target=false;
    int tr=-1, tc=-1;

public:
    Robot_BlindBot() : RobotBase(2,5,flamethrower)
    {
        std::srand((unsigned)std::time(nullptr));
    }

    virtual void get_radar_direction(int& out) override
    {
        out = 3; // Always looks right
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
                return;
            }
        }
    }

    virtual bool get_shot_location(int& sr,int& sc) override
    {
        if (!has_target) return false;
        sr = tr;
        sc = tc;
        return true;
    }

    virtual void get_move_direction(int& d,int& dist) override
    {
        d = (std::rand()%8)+1;
        dist=1;
    }
};

extern "C" RobotBase* create_robot() { return new Robot_BlindBot(); }
