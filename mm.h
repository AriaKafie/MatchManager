
#ifndef MM_H
#define MM_H

#include <fstream>
#include <sstream>

#include "engine.h"
#include "position.h"
#include "uci.h"

#define DEFAULT_TIME 100
#define DEFAULT_THREADS 1

class Match
{
public:
    Match(std::string path_1, std::string path_2, int time, int id, std::string fenpath)
        : e1(path_1, time, id),
          e2(path_2, time, id), m_id(id), draws(0), failed(false)
    {
        log.open(std::string("logs\\")+e1.name()+"_"+e2.name()+"_"+std::to_string(time)+"_id"+std::to_string(m_id)+".txt");
        fenfile.open(fenpath);
    }

    ~Match() { log.close(); fenfile.close(); e1.kill(); e2.kill(); }

    void run_games();

    Engine e1;
    Engine e2;
    int draws;

private:
    std::string uci_to_pgn(const std::string& uci);

    Position pos;
    int m_id;
    bool failed;
    std::ofstream log;
    std::ifstream fenfile;
};

inline std::string Match::uci_to_pgn(const std::string& uci)
{
    std::string pgn, fen, token;
    Position p;
    std::istringstream is(uci);

    while (is >> token)
    {
        if (token == "startpos")
        {
            fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
            is >> token;
            break;
        }
        else if (token == "fen")
        {
            for (;is >> token && token != "moves"; fen += token + " ");
            break;
        }
    }

    p.set(fen);

    pgn += "[White \"" + (p.white_to_move() ? e1.name() : e2.name()) + "\"]\n";
    pgn += "[Black \"" + (p.white_to_move() ? e2.name() : e1.name()) + "\"]\n";
    pgn += "[FEN \"" + fen.substr(0, fen.size() - 1) + "\"]\n";

    for (int movenum = 1; is >> token;)
    {
        Move move = uci_to_move(token, p);

        if (p.white_to_move())
        {
            pgn += std::to_string(movenum) + ". " + move_to_san(move, p) + " ";
            p.do_move(move);
        }
        else
        {
            if (movenum == 1)
                pgn += "1... " + move_to_san(move, p) + " ";
            else
                pgn += move_to_san(move, p) + " ";

            p.do_move(move);
            movenum++;
        }
    }

    return pgn;
}

#endif