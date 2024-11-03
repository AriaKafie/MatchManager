
#include "mm.h"

#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <sstream>
#include <fstream>
#include <vector>

#include "bitboard.h"
#include "engine.h"
#include "position.h"
#include "uci.h"

void handle_stop(bool *sigstop)
{
    std::string in;

    do
        std::getline(std::cin, in);
    while (in != "stop");

    *sigstop = true;
}

void refresh(const std::string& fen, const Position& pos, const Engine& e1, const Engine& e2, int draws)
{
    system("cls");
    printf("%s\n%s\n%s: %d\n%s: %d\ndraws: %d\n",
           fen.c_str(), pos.to_string().c_str(), e1.name().c_str(), e1.wins, e2.name().c_str(), e2.wins, draws);
    std::cout << std::endl;
}

void Match::run_games()
{
    bool sigstop = false;
    
    std::thread t([&sigstop]() { handle_stop(&sigstop); });
    t.detach();

    std::vector<std::string> fens;
    for (std::string fen; std::getline(fenfile, fen); fens.push_back(fen));

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(fens.begin(), fens.end(), g);

    for (const std::string& fen : fens)
    {
        pos.set(fen);

        Color e1_color = pos.side_to_move(), e2_color = !e1_color;

        e1.write_to_stdin("ucinewgame\nposition fen " + fen + "\n");
        e2.write_to_stdin("ucinewgame\nposition fen " + fen + "\n");

        while (!sigstop)
        {
            Engine &engine = pos.side_to_move() == e1_color ? e1 : e2;

            system("cls");
            refresh(fen, pos, e1, e2, draws);

            std::string movestr = engine.best_move();
            Move best_move = uci_to_move(movestr, pos);

            if (best_move == NULLMOVE)
            {
                std::cout << "invalid move: " << movestr << std::endl;
                std::exit(1);
            }

            e1.write_to_stdin("moves " + movestr + "\n");
            e2.write_to_stdin("moves " + movestr + "\n");

            pos.do_move(best_move);

            if (GameState g = pos.game_state(); g != ONGOING)
            {
                refresh(fen, pos, e1, e2, draws);

                if      (g == MATE) engine.wins++;
                else if (g == DRAW) draws++;

                break;
            }
        }

        if (sigstop)
        {
            e1.kill();
            e2.kill();
            return;
        }
    }
}

int main()
{
    Bitboards::init();

    Match m("C:\\Users\\14244\\Desktop\\chess\\mm\\engines\\tt256.exe",
            "C:\\Users\\14244\\Desktop\\chess\\mm\\engines\\tt256.exe",
            50, 50, 0, "C:\\Users\\14244\\Desktop\\chess\\mm\\lc01k.txt");

    m.run_games();

    std::cout << "Exiting successfully\n" << std::endl;

    return 0;
}