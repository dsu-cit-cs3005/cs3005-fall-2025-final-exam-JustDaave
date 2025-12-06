#include "RobotBase.h"
#include "RadarObj.h"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <dlfcn.h>
#include <cstdlib>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

static std::mt19937 rng(std::random_device{}());
static int rand_int(int a,int b){ return std::uniform_int_distribution<>(a,b)(rng); }
static inline bool in_bounds(int r,int c,int R,int C){ return r>=0 && c>=0 && r<R && c<C; }
static int clamp(int v,int lo,int hi){ return std::max(lo,std::min(hi,v)); }



// ---------------------------------
// GAME CONSTANTS
// ---------------------------------
static constexpr int ROWS = 20;
static constexpr int COLS = 20;
static constexpr int MAX_ROUNDS = 100;
static constexpr int MOUNDS = 10;
static constexpr int PITS   = 5;
static constexpr int FLAMERS = 5;
static constexpr bool LIVE = true;

struct BotHandle {
    void* so = nullptr;
    RobotBase* robot = nullptr;
};

struct Cell {
    char c='.';
};

// ------------------------
// PLACE RANDOM ITEMS
// ------------------------
static void place_random(std::vector<std::vector<Cell>>& b, char t, int count){
    while(count--){
        int r,c;
        do{
            r=rand_int(0, ROWS-1);
            c=rand_int(0, COLS-1);
        } while(b[r][c].c!='.');
        b[r][c].c = t;
    }
}

// ------------------------
// BOARD DISPLAY
// ------------------------
static void print_board(const auto& board, const auto& bots, int round){
    std::cout << "\n=========== Round " << round << " ===========\n\n";

    std::cout << "   ";
    for(int c=0;c<COLS;++c)
        std::cout << (c<10?" ":"") << c << ' ';
    std::cout << "\n";

    for(int r=0;r<ROWS;++r){
        std::cout << (r<10?" ":"") << r << " ";
        for(int c=0;c<COLS;++c)
            std::cout << ' ' << board[r][c].c << ' ';
        std::cout << "\n";
    }

    for(auto& bh: bots)
        if(bh.robot)
            std::cout << bh.robot->print_stats() << "\n";

    std::cout << "\n";
}

// ------------------------
// RADAR SCAN
// ------------------------
static std::vector<RadarObj> do_radar_scan(const auto& board, RobotBase* bot, int dir){
    std::vector<RadarObj> out;
    int br, bc; bot->get_current_location(br,bc);

    // 0 = scan all surrounding cells
    if(dir == 0){
        for(int dr=-1; dr<=1; ++dr)
            for(int dc=-1; dc<=1; ++dc)
                if(dr || dc){
                    int nr = br+dr, nc = bc+dc;
                    if(in_bounds(nr,nc,ROWS,COLS))
                        out.emplace_back(board[nr][nc].c,nr,nc);
                }
        return out;
    }

    auto [dr,dc] = directions[dir];
    for(int nr=br+dr, nc=bc+dc; in_bounds(nr,nc,ROWS,COLS); nr+=dr,nc+=dc){
        for(int w=-1; w<=1; ++w){
            int wr = nr + (dc?0:w);
            int wc = nc + (dr?0:w);
            if(in_bounds(wr,wc,ROWS,COLS))
                out.emplace_back(board[wr][wc].c, wr, wc);
        }
    }
    return out;
}

// ------------------------
// DAMAGE SYSTEM
// ------------------------
static int roll_damage(WeaponType w){
    switch(w){
        case railgun:      return rand_int(10,20);
        case hammer:       return rand_int(50,60);
        case grenade:      return rand_int(10,40);
        case flamethrower: return rand_int(30,50);
        default:           return 0;
    }
}

static void apply_damage(RobotBase* t, int raw){
    double mult = std::max(0.0, 1.0 - t->get_armor()*0.1);
    int dmg = (int)std::round(raw * mult);
    t->take_damage(dmg);
    t->reduce_armor(1);
}

// ------------------------
// MAIN
// ------------------------
int main(){
    // -----------------------------------------
    // BUILD BOARD
    // -----------------------------------------
    std::vector<std::vector<Cell>> board(ROWS, std::vector<Cell>(COLS));

    place_random(board,'M',MOUNDS);
    place_random(board,'P',PITS);
    place_random(board,'F',FLAMERS);

    // -----------------------------------------
    // LOAD ROBOTS 
    // -----------------------------------------
    std::vector<BotHandle> bots;
    std::vector<std::string> so_files;

    for(auto& f : fs::directory_iterator(".")){
        if(!f.is_regular_file()) continue;

        std::string name = f.path().filename().string();
        if(name.starts_with("Robot_") && f.path().extension()==".cpp"){
            std::string so = f.path().stem().string() + ".so";
            std::string cmd = "g++ -shared -fPIC -o " + so + " " +
                              name + " RobotBase.o -I. -std=c++20";

            std::cout << "Compiling: " << name << "\n";
            if(std::system(cmd.c_str()) == 0)
                so_files.push_back(so);
        }
    }

    for(auto& so : so_files){
        void* h = dlopen(so.c_str(), RTLD_LAZY);
        if(!h){ std::cerr << dlerror() << "\n"; continue; }

        auto create = (RobotFactory)dlsym(h, "create_robot");
        if(!create){ dlclose(h); continue; }

        RobotBase* r = create();
        if(!r){ dlclose(h); continue; }

        bots.push_back({h,r});
    }

    if(bots.empty()){
        std::cerr << "No robots loaded.\n";
        return 1;
    }

    // -----------------------------------------
    // PLACE ROBOTS
    // -----------------------------------------
    for(size_t i=0; i<bots.size(); ++i){
        int r,c;
        do{
            r=rand_int(0,ROWS-1);
            c=rand_int(0,COLS-1);
        } while(board[r][c].c!='.');

        board[r][c].c='R';

        auto* b = bots[i].robot;
        b->m_character = 'A' + i;
        b->m_name = "Robot_" + std::string(1,b->m_character);
        b->set_boundaries(ROWS,COLS);
        b->move_to(r,c);
    }

    // Helper: find bot at location
    auto find_bot = [&](int r,int c){
        for(auto& bh: bots){
            auto* b = bh.robot;
            if(!b || b->get_health()<=0) continue;
            int br,bc; b->get_current_location(br,bc);
            if(br==r && bc==c) return b;
        }
        return (RobotBase*)nullptr;
    };

    // -----------------------------------------
    // MAIN GAME LOOP
    // -----------------------------------------
    int stagnation = 0;

    for(int round=1; round<=MAX_ROUNDS; ++round){
        // Check if only one bot alive
        int alive = 0;
        RobotBase* last = nullptr;

        for(auto& bh: bots)
            if(bh.robot && bh.robot->get_health()>0)
                alive++, last = bh.robot;

        if(alive <= 1){
            print_board(board,bots,round);
            if(last)
                std::cout << "Winner: " << last->m_name << "\n";
            else
                std::cout << "All robots dead.\n";
            break;
        }

        bool progress = false;

        // --------------------------
        // ROBOT TURN LOOP
        // --------------------------
        for(auto& bh: bots){
            RobotBase* b = bh.robot;
            if(!b) continue;

            int br,bc;
            b->get_current_location(br,bc);

            if(b->get_health()==0){
                board[br][bc].c='X';
                continue;
            }

            if(LIVE)
                print_board(board,bots,round);

            // Radar
            int dir=0;
            b->get_radar_direction(dir);
            dir = clamp(dir,0,8);

            auto radar = do_radar_scan(board,b,dir);
            b->process_radar_results(radar);

            // --------------------------
            // SHOOTING
            // --------------------------
            int sr,sc;
            if(b->get_shot_location(sr,sc)){
                WeaponType w=b->get_weapon();
                int raw = roll_damage(w);

                auto damage_at = [&](int r,int c){
                    if(auto* t = find_bot(r,c)){
                        apply_damage(t,raw);
                        progress = true;
                        if(t->get_health()<=0)
                            board[r][c].c='X';
                    }
                };

                int dr = (sr > br) - (sr < br);
                int dc = (sc > bc) - (sc < bc);
                if(dr==0 && dc==0) dc = 1;

                if(w==railgun){
                    for(int nr=br+dr, nc=bc+dc;
                        in_bounds(nr,nc,ROWS,COLS);
                        nr+=dr,nc+=dc)
                        damage_at(nr,nc);
                }
                else if(w==flamethrower){
                    for(int s=1;s<=4;++s){
                        int cr=br+dr*s, cc=bc+dc*s;
                        for(int w2=-1; w2<=1; ++w2){
                            int wr = cr + (dc?0:w2);
                            int wc = cc + (dr?0:w2);
                            if(in_bounds(wr,wc,ROWS,COLS))
                                damage_at(wr,wc);
                        }
                    }
                }
                else if(w==grenade && b->get_grenades()>0){
                    b->decrement_grenades();
                    for(int dr2=-1; dr2<=1; ++dr2)
                        for(int dc2=-1; dc2<=1; ++dc2)
                            if(in_bounds(sr+dr2, sc+dc2, ROWS,COLS))
                                damage_at(sr+dr2, sc+dc2);
                }
                else if(w==hammer){
                    damage_at(sr,sc);
                }
            }

            // --------------------------
            // MOVEMENT
            // --------------------------
            else {
                int md=0, dist=0;
                b->get_move_direction(md,dist);

                md = clamp(md,1,8);
                dist = clamp(dist,0, b->get_move_speed());
                auto [dr,dc] = directions[md];

                int cr=br, cc=bc;

                for(int s=0; s<dist; ++s){
                    int nr = cr+dr, nc = cc+dc;
                    if(!in_bounds(nr,nc,ROWS,COLS)) break;

                    char ch = board[nr][nc].c;

                    if(ch=='M' || ch=='X' || find_bot(nr,nc))
                        break;

                    if(ch=='P'){
                        board[cr][cc].c='.';
                        cr=nr; cc=nc;
                        board[cr][cc].c='P';
                        b->move_to(cr,cc);
                        b->disable_movement();
                        progress = true;
                        break;
                    }

                    if(ch=='F'){
                        apply_damage(b,roll_damage(flamethrower));
                        progress = true;
                        if(b->get_health() <= 0){
                            board[cr][cc].c='.';
                            board[nr][nc].c='X';
                            b->move_to(nr,nc);
                            break;
                        }
                    }

                    board[cr][cc].c='.';
                    cr=nr; cc=nc;
                    board[cr][cc].c='R';
                    b->move_to(cr,cc);
                    progress = true;
                }
            }

            if(LIVE)
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }

        // -----------------------------------------
        // STAGNATION CHECK
        // -----------------------------------------
        stagnation = progress ? 0 : stagnation + 1;

        if(stagnation >= 30){
            print_board(board,bots,MAX_ROUNDS);

            RobotBase* win = nullptr;
            int best = -1;
            for(auto& bh : bots){
                auto* r = bh.robot;
                if(r && r->get_health() > best){
                    best = r->get_health();
                    win = r;
                }
            }

            if(win)
                std::cout << "Winner by health (stagnation): " << win->m_name << "\n";
            else
                std::cout << "Stalemate.\n";

            goto after_loop;
        }
    }

    // -----------------------------------------
    // END OF MAX_ROUNDS → PICK WINNER
    // -----------------------------------------
    {
        RobotBase* winner = nullptr;
        int bestScore = -1;

        for(auto& bh : bots){
            auto* r = bh.robot;
            if(!r) continue;

            int score = r->get_health() + r->get_armor();
            if(score > bestScore){
                bestScore = score;
                winner = r;
            }
        }

        std::cout << "\n=========== FINAL RESULT ===========\n";
        if(winner)
            std::cout << "Winner (health+armor): " << winner->m_name
                      << " | Score = " << bestScore << "\n";
        else
            std::cout << "No winner.\n";
    }

after_loop:

    // -----------------------------------------
    // CLEANUP
    // -----------------------------------------
    for(auto& bh: bots){
        delete bh.robot;
        if(bh.so) dlclose(bh.so);
    }
}
