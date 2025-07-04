
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include "mm.h"

#include <ctime>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <fstream>
#include <vector>

#include "bitboard.h"
#include "engine.h"
#include "position.h"
#include "stats.h"

Color random_color() {
    static std::mt19937_64 rng(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    return rng() & 1;
}

template<typename T>
bool get_opt(const std::string& option, T& var, int argc, char *argv[])
{
    std::string opt = "-" + option, args, token;

    for (int i = 3; i < argc; i++)
        args += std::string(argv[i]) + " ";

    for (std::istringstream is(args); is >> token;)
    {
        if (token == opt)
            return bool(is >> var);

        if (token.find(opt) == 0)
        {
            std::istringstream i(token.substr(opt.length()));
            return bool(i >> var);
        }
    }

    return true;
}

std::string time_()
{
    std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::tm *local_time = std::localtime(&current_time);

    std::ostringstream time_stream;
    time_stream << std::put_time(local_time, "%H:%M:%S");

    return time_stream.str();
}

void handle_stdin(Status *status)
{
    std::string in;

    do
    {
        std::getline(std::cin, in);
        std::istringstream is(in);
        is >> in;

        if (in == "pause")
            *status = PAUSE;
        else if (in == "go")
            *status = GO;

    } while (in != "stop" && in != "quit");

    *status = QUIT;
}

void run_match(Match *match, Status *status) {
    match->run_games(status);
}

void Match::run_games(Status *status)
{
    for (int i = 0; i < fens.size(); i++)
    {
        std::string fen = fens[i];

        printf("%s Match %d %s %d %s %d Draws %d (%+d +/- %d) Game %d/%llu\n",
               time_().c_str(), m_id, e1.name().c_str(), e1.wins, e2.name().c_str(), e2.wins, draws,
               (int)elo_diff  (e1.wins, e2.wins, draws),
               (int)elo_margin(e1.wins, e2.wins, draws),
               i, fens.size());

        pos.set(fen);

        Color e1_color = random_color(), e2_color = !e1_color;

        std::string game_string = "position fen " + fen;

        e1.write_to_stdin("ucinewgame\n" + game_string + "\n");
        e2.write_to_stdin("ucinewgame\n" + game_string + "\n");

        game_string += " moves";
        log << game_string;

        while (*status != QUIT)
        {
            for (;*status == PAUSE; Sleep(100));

            Engine &engine = pos.side_to_move() == e1_color ? e1 : e2;

            std::string movestr = engine.best_move();
            Move        move    = pos.uci_to_move(movestr);

            if (move == Move::null())
            {
                log                                                      << std::endl
                    << uci_to_pgn(game_string, e1_color, e2_color)       << std::endl
                    << pos.to_string()                                   << std::endl
                    << engine.name() << ": " << movestr << " <- Invalid" << std::endl;

                failed = true;
                break;
            }

            game_string += " " + movestr;
            log << " " << movestr;

            e1.write_to_stdin(game_string + "\n");
            e2.write_to_stdin(game_string + "\n");

            pos.do_move(move);

            if (GameState g = pos.game_state(); g != ONGOING)
            {
                if (g == MATE)
                    engine.wins++;
                else
                    draws++;

                log << std::endl
                    << uci_to_pgn(game_string, e1_color, e2_color) << std::endl
                    << (g == MATE       ? "Checkmate"
                      : g == STALEMATE  ? "Stalemate"
                      : g == REPETITION ? "Repetition"
                      : g == FIFTY_MOVE ? "Fifty-move rule" : "?") << std::endl
                    << e1.name() << ": " << e1.wins << " " << e2.name() << ": " << e2.wins << " Draws: " << draws << "\n" << std::endl;

                break;
            }
        }

        if (*status == QUIT || failed) break;
    }

    e1.kill();
    e2.kill();

    std::cout << "Match " << m_id << (failed ? ": Engine error" : ": Done") << std::endl;
}

int main(int argc, char *argv[])
{
    Bitboards::init();
    Position::init();

    std::string name_1, name_2, fenpath = "lc01k.txt";
    int time    = DEFAULT_TIME;
    int threads = DEFAULT_THREADS;

    std::string tokens, token;

    for (int i = 1; i < argc; i++)
        tokens += std::string(argv[i]) + " ";

    std::istringstream args(tokens);

    if (!(args >> name_1 >> name_2)
        || !get_opt("time", time, argc, argv)
        || !get_opt("threads", threads, argc, argv)
        || !get_opt("fen", fenpath, argc, argv))
    {
        std::cout << "Usage: MatchManager <engine1> <engine2> [-threads <threads>] [-time <milliseconds>] [-fen <fenfile>]" << std::endl;
        return 1;
    }

    std::string path_1 = std::string("engines\\") + name_1 + ".exe";
    std::string path_2 = std::string("engines\\") + name_2 + ".exe";

    std::vector<Match*> matches;
    std::vector<std::thread> thread_pool;

    Status status = GO;
    std::thread t(handle_stdin, &status);
    t.detach();

    for (int id = 0; id < threads; id++)
    {
        matches.push_back(new Match(path_1, path_2, time, id, fenpath));
        thread_pool.emplace_back(run_match, matches.back(), &status);
    }

    for (std::thread& thread : thread_pool)
        thread.join();

    int e1_wins = 0, e2_wins = 0, draws = 0;

    for (Match *m : matches)
    {
        e1_wins += m->e1.wins;
        e2_wins += m->e2.wins;
        draws += m->draws;

        delete m;
    }

    int total = e1_wins + e2_wins + draws, decisive = total - draws;
    
    double e1_winrate = decisive > 0 ? (double)e1_wins / decisive : 0.0;
    double e2_winrate = decisive > 0 ? (double)e2_wins / decisive : 0.0;

    printf("+-----------------+-------+----------+\n");
    printf("|     Outcome     |   #   | Win Rate |\n");
    printf("+-----------------+-------+----------+\n");

    printf("| %-16s|%6d |%8.2f%% |\n", name_1.c_str(), e1_wins, e1_winrate * 100);
    printf("| %-16s|%6d |%8.2f%% |\n", name_2.c_str(), e2_wins, e2_winrate * 100);
    printf("| Draws           |%6d |          |\n", draws);
    printf("| Total           |%6d |%6d ms |\n+-----------------+-------+----------+\n", total, time);

    printf("%s is %f (+/- %f) elo away from %s\n",
        name_1.c_str(), elo_diff(e1_wins, e2_wins, draws), elo_margin(e1_wins, e2_wins, draws), name_2.c_str());
}
