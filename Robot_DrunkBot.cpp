#include "RobotBase.h"
#include <cstdlib>
#include <ctime>

class Robot_DrunkBot : public RobotBase
{
private:
    bool has_target=false;
    int tr=-1, tc=-1;

public:
    Robot_DrunkBot() : RobotBase(2,5,flamethrower)
    {
        std::srand((unsigned)std::time(nullptr));
    }

    virtual void get_radar_direction(int& out) override
    {
        out = (std::rand()%8)+1;
    }

    virtual void process_radar_results(const std::vector<RadarObj>& r) override
    {
        has_target=false;
        for (auto& o : r)
        {
            if (o.m_type=='R')
            {
                has_target=true;
                tr=o.m_row;
                tc=o.m_col;
            }
        }
    }

    virtual bool get_shot_location(int& sr,int& sc) override
    {
        if (!has_target) return false;

        // Drunk guess
        sr = tr + (std::rand()%3 - 1);
        sc = tc + (std::rand()%3 - 1);
        return true;
    }

    virtual void get_move_direction(int& d,int& dist) override
    {
        d = (std::rand()%8)+1;
        dist = (std::rand()%2)+1; // 1–2 tiles
    }
};

extern "C" RobotBase* create_robot() { return new Robot_DrunkBot(); }
