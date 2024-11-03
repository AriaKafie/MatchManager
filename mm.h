
#ifndef MM_H
#define MM_H

#include <fstream>

#include "engine.h"
#include "position.h"

class Match
{
public:
    Match(std::string path_1, std::string path_2, int time_1, int time_2, int id, std::string fenpath)
        : e1(path_1, time_1),
          e2(path_2, time_2), m_id(id), draws(0)
    {
        log.open(std::to_string(time_1) + std::to_string(time_2) + "m_id" + std::to_string(m_id));
        fenfile.open(fenpath);
    }

    ~Match() { log.close(); fenfile.close(); }

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