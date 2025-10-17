
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include "mm.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "bitboard.h"
#include "engine.h"
#include "args.h"
#include "position.h"
#include "stats.h"

Color random_color() {
    static std::mt19937_64 rng(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    return rng() & 1;
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

void Match::run(Status *status)
{
    for (int i = 0; i < fens.size(); i++)
    {
        std::string fen = fens[i];

        printf
        (   
            "%s Match %d %s %d %s %d Draws %d (%+d +/- %d) Game %d/%llu\n",
            time_().c_str(),
            m_id,
            e1.name().c_str(),
            e1.wins,
            e2.name().c_str(),
            e2.wins,
            draws,
            (int)elo_diff  (e1.wins, e2.wins, draws),
            (int)elo_margin(e1.wins, e2.wins, draws),
            i,
            fens.size()
        );

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

            Engine& engine = pos.side_to_move() == e1_color ? e1 : e2;

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

    verify_args(argc, argv);
    
    std::string engine1_path = get_required("engine1", argc, argv);
    std::string engine2_path = get_required("engine2", argc, argv);
    int time = std::stoi(get_with_default("time", argc, argv, "100"));
    int threads = std::stoi(get_with_default("threads", argc, argv, "1"));
    std::string fen_file = get_with_default("fen_file", argc, argv, "lc01k.txt");

    std::vector<Match*> matches;
    std::vector<std::thread> thread_pool;

    Status status = GO;
    std::thread t(handle_stdin, &status);
    t.detach();

    for (int id = 0; id < threads; id++)
    {
        matches.push_back(new Match(engine1_path, engine2_path, time, id, fen_file));
        thread_pool.emplace_back(&Match::run, matches.back(), &status);
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
    double diff = elo_diff(e1_wins, e2_wins, draws);
    double margin = elo_margin(e1_wins, e2_wins, draws);

    printf(R"(
+-----------------+-------+----------+
|     Outcome     |   #   | Win Rate |
+-----------------+-------+----------+
| %-16s|%6d |%8.2f%% |
| %-16s|%6d |%8.2f%% |
| Draws           |%6d |          |
| Total           |%6d |%6d ms |
+-----------------+-------+----------+
%s is %f (+/- %f) elo ahead of %s
)",
    engine1_path.c_str(), e1_wins, e1_winrate * 100,
    engine2_path.c_str(), e2_wins, e2_winrate * 100,
    draws,
    total, time,
    engine1_path.c_str(), diff, margin, engine2_path.c_str());
}
