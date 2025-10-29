
#ifndef MM_H
#define MM_H

#include <algorithm>
#include <fstream>
#include <random>
#include <sstream>

#include "engine.h"
#include "position.h"
#include "uci.h"

enum Status { PAUSE, GO, QUIT };

class Match
{
public:
    Match(std::string path_1, std::string path_2, int time, int id, std::string fenpath)
        : e1(path_1, time, id), e2(path_2, time, id), m_id(id), failed(false), draws(0)
    {
        e1.write_to_stdin("noverbose\nuci\nisready\n");
        e2.write_to_stdin("noverbose\nuci\nisready\n");

        log.open("logs\\"+e1.name()+"_"+e2.name()+"_"+std::to_string(time)+"_id"+std::to_string(m_id)+".txt");
        
        std::string fen;
        for (std::ifstream fenfile(fenpath); std::getline(fenfile, fen); fens.push_back(fen));

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(fens.begin(), fens.end(), g);
    }

    ~Match() { log.close(); }

    void run(Status *status);

    Engine e1;
    Engine e2;
    int    draws;

private:
    Position                 pos;
    int                      m_id;
    bool                     failed;
    std::ofstream            log;
    std::vector<std::string> fens;
};

#endif