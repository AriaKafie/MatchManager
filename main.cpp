
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <fstream>

#include "bitboard.h"
#include "engine.h"
#include "position.h"
#include "uci.h"

int main()
{
    Bitboards::init();

    std::ofstream log("log.txt");

    Engine e1("C:\\Users\\14244\\Desktop\\chess\\mm\\engines\\allcaps.exe", 100);
    Engine e2("C:\\Users\\14244\\Desktop\\chess\\mm\\engines\\allcaps.exe", 100);

    Position    pos;
    std::string movestr;
    Move        best_move;

    while (1)
    {
        pos.set();
        e1.write_to_stdin("ucinewgame\nposition startpos\n");
        e2.write_to_stdin("ucinewgame\nposition startpos\n");

        while (1)
        {
            system("cls");
            std::cout << pos.to_string() << std::endl;

            log << pos.to_string() << std::endl;

            movestr = e1.best_move();
            best_move = uci_to_move(movestr, pos);

            log << "engine1: " << movestr << std::endl;

            if (best_move == NULLMOVE)
            {
                std::cout << "invalid move: " << movestr << std::endl;
                return 1;
            }

            e1.write_to_stdin("moves " + movestr + "\n");
            e2.write_to_stdin("moves " + movestr + "\n");

            pos.do_move(best_move);
            system("cls");
            std::cout << pos.to_string() << std::endl;

            log << pos.to_string() << std::endl;

            if (GameState g = pos.game_state(); g != ONGOING)
            {
                e1.wins++;
                break;
            }

            movestr = e2.best_move();
            best_move = uci_to_move(movestr, pos);

            log << "engine2: " << movestr << std::endl;

            if (best_move == NULLMOVE)
            {
                std::cout << "invalid move: " << movestr << std::endl;
                return 1;
            }

            e1.write_to_stdin("moves " + movestr + "\n");
            e2.write_to_stdin("moves " + movestr + "\n");

            pos.do_move(best_move);
            system("cls");
            std::cout << pos.to_string() << std::endl;

            if (GameState g = pos.game_state(); g != ONGOING)
            {
                e2.wins++;
                break;
            }
        }
    }

    std::cout << "done" << std::endl;
}