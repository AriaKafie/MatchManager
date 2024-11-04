
#ifndef MM_H
#define MM_H

#include <fstream>

#include "engine.h"
#include "position.h"

#define DEFAULT_TIME 100
#define DEFAULT_THREADS 1

class Match
{
public:
    Match(std::string path_1, std::string path_2, int time, int id, std::string fenpath)
        : e1(path_1, time),
          e2(path_2, time), m_id(id), draws(0)
    {
        log.open(e1.name()+"_"+e2.name()+"_"+std::to_string(time)+"_id_"+std::to_string(m_id)+".txt");
        fenfile.open(fenpath);
    }

    ~Match() { log.close(); fenfile.close(); e1.kill(); e2.kill(); }

    void run_games();

private:
    Position pos;

    Engine e1;
    Engine e2;
    int    m_id;
    int    draws;
    std::ofstream log;
    std::ifstream fenfile;
};

#endif