
#include "mm.h"

#include <algorithm>
#include <iomanip>
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

static volatile bool stop;

void await_stop()
{
    stop = false;

    std::string in;
    do
        std::getline(std::cin, in);
    while (in != "stop");

    stop = true;
}

void run_match(Match *match) {
    match->run_games();
}

void Match::run_games()
{
    std::vector<std::string> fens;
    for (std::string fen; std::getline(fenfile, fen); fens.push_back(fen));

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(fens.begin(), fens.end(), g);

    for (int i = 0; i < fens.size(); i++)
    {
        std::string fen = fens[i];
        printf("Match %d: %s: %d %s: %d draws: %d game: %d/%d\n",
               m_id, e1.name().c_str(), e1.wins, e2.name().c_str(), e2.wins, draws, i + 1, fens.size());

        log << "\n" << fen << std::endl;

        pos.set(fen);

        Color e1_color = pos.side_to_move(), e2_color = !e1_color;

        e1.write_to_stdin("ucinewgame\nposition fen " + fen + "\n");
        e2.write_to_stdin("ucinewgame\nposition fen " + fen + "\n");

        while (!stop)
        {
            Engine &engine = pos.side_to_move() == e1_color ? e1 : e2;

            std::string movestr = engine.best_move();
            Move best_move = uci_to_move(movestr, pos);

            if (best_move == NULLMOVE)
            {
                log << pos.to_string() << std::endl << engine.name() << ": " << movestr << " <- Invalid" << std::endl;
                return;
            }

            log << movestr << " ";

            e1.write_to_stdin("position fen " + pos.fen() + " moves " + movestr + "\n");
            e2.write_to_stdin("position fen " + pos.fen() + " moves " + movestr + "\n");

            pos.do_move(best_move);

            if (GameState g = pos.game_state(); g != ONGOING)
            {
                if (g == MATE) engine.wins++;
                else           draws++;

                log << "\n"
                    << e1.name() << ": " << e1.wins << " " << e2.name() << ": " << e2.wins << "draws: " << draws << std::endl;

                break;
            }
        }

        if (stop) return;
    }

    std::cout << "Match " << m_id << " done" << std::endl;
}

int main(int argc, char **argv)
{
    std::string name_1, name_2;
    int time    = DEFAULT_TIME;
    int threads = DEFAULT_THREADS;

    std::string tokens, token;
    for (int i = 1; i < argc; tokens += std::string(argv[i++]) + " ");

    std::istringstream args(tokens);

    while (args >> token)
    {
        if (token.find("-time") != std::string::npos)
        {
            if (token == "-time")
                args >> time;
            else
                time = std::stoi(token.substr(std::string("-time").size()));
        }
        else if (token.find("-threads") != std::string::npos)
        {
            if (token == "-threads")
                args >> threads;
            else
                threads = std::stoi(token.substr(std::string("-threads").size()));
        }
        else
        {
            name_1 = token;
            args >> name_2;
        }
    }

    std::string path_1 = std::string("C:\\Users\\14244\\Desktop\\chess\\mm\\engines\\") + name_1 + ".exe",
                path_2 = std::string("C:\\Users\\14244\\Desktop\\chess\\mm\\engines\\") + name_2 + ".exe";

    Bitboards::init();
    Position::init();

    std::vector<Match*> matches;
    std::vector<std::thread> thread_pool;

    for (int id = 0; id < threads; id++)
    {
        Match* m = new Match(path_1, path_2, time, id, "lc01k.txt");
        matches.push_back(m);
        thread_pool.emplace_back([m]() { run_match(m); });
    }

    std::thread t([]() { await_stop(); });
    t.detach();

    for (std::thread &t : thread_pool) t.join();

    int e1_wins = 0, e2_wins = 0, draws = 0;

    for (Match *m : matches)
    {
        e1_wins += m->e1.wins;
        e2_wins += m->e2.wins;
        draws += m->draws;

        delete m;
    }

    int total = e1_wins + e2_wins + draws, decisive = total - draws;
    
    double e1_winrate = decisive > 0 ? double(e1_wins) / decisive : 0.0;
    double e2_winrate = decisive > 0 ? double(e2_wins) / decisive : 0.0;

    std::cout << "+-----------------+-------+----------+" << std::endl;
    std::cout << "|     Outcome     |   #   | Win Rate |" << std::endl;
    std::cout << "+-----------------+-------+----------+" << std::endl;

    printf("| %-16s|%6d |%8.2f%% |\n", name_1.c_str(), e1_wins, e1_winrate * 100);
    printf("| %-16s|%6d |%8.2f%% |\n", name_2.c_str(), e2_wins, e2_winrate * 100);
    printf("| Draws           |%6d |          |\n", draws);
    printf("| Decisive        |%6d |          |\n", decisive);
    printf("| Total           |%6d |          |\n+-----------------+-------+----------+\n\n", total);
}
