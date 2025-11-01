
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

uint64_t unix_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

Color random_color() {
    static std::mt19937_64 rng(unix_ms());
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
        std::cin >> in;

        if (in == "stop")
            *status = STOP;
        else if (in == "go")
            *status = GO;

    } while (in != "quit");

    *status = QUIT;
}

void Match::run(Status *status)
{
    uint64_t start_time = unix_ms();

    for (int i = 0; i < fens.size(); i++)
    {
        std::string fen = fens[i];

        uint64_t elapsed = unix_ms() - start_time;
        uint64_t time_per_game = elapsed / std::max(1, i);
        uint64_t eta_seconds = time_per_game * (fens.size() - i) / 1000;

        int hours = eta_seconds / 3600;
        int minutes = (eta_seconds % 3600) / 60;
        int seconds = eta_seconds % 60;

        std::ostringstream eta;
        eta << std::setw(2) << std::setfill('0') << hours   << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << seconds;

        printf
        (
            "%s Match %d %s %d %s %d Draws %d (%+d +/- %d) Game %d/%llu ETA %s\n",
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
            fens.size(),
            eta.str().c_str()
        );

        pos.set(fen);

        Color e1_color = random_color(), e2_color = !e1_color;

        std::stringstream uci, pgn;

        uci << "position fen " << fen << " ";

        pgn << "[White \"" << (e1_color == WHITE ? e1.name() : e2.name()) << "\"]\n"
            << "[Black \"" << (e1_color == BLACK ? e1.name() : e2.name()) << "\"]\n"
            << "[FEN \"" << fen << "\"]\n";

        if (pos.black_to_move()) pgn << "1... ";

        e1.write_to_stdin("ucinewgame\n" + uci.str() + "\n");
        e2.write_to_stdin("ucinewgame\n" + uci.str() + "\n");

        uci << "moves ";
        log << uci.str() << std::flush;

        for (int pgn_num = 1; *status != QUIT;)
        {
            for (;*status == STOP; Sleep(100));

            Engine& engine = pos.side_to_move() == e1_color ? e1 : e2;

            std::string uci_move = engine.best_move();
            Move move = uci_to_move(uci_move, pos);

            if (move == Move::null())
            {
                log << std::endl
                    << pgn.str() << std::endl
                    << pos.to_string() << std::endl
                    << engine.name() << ": " << uci_move << " <- Invalid" << std::endl;

                failed = true;
                break;
            }

            if (pos.white_to_move())
                pgn << pgn_num << ". ";
            pgn << move_to_san(move, pos) << " ";
            if (pos.black_to_move())
                pgn_num++;

            uci << uci_move << " ";
            log << uci_move << " " << std::flush;

            e1.write_to_stdin(uci.str() + "\n");
            e2.write_to_stdin(uci.str() + "\n");

            pos.do_move(move);

            if (GameState g = pos.game_state(); g != ONGOING)
            {
                if (g == MATE) {
                    engine.wins++;
                    pgn << (pos.white_to_move() ? "0-1" : "1-0");
                } else {
                    draws++;
                    pgn << "1/2-1/2";
                }

                log << std::endl
                    << pgn.str() << std::endl
                    << (g == MATE       ? "Checkmate"
                      : g == STALEMATE  ? "Stalemate"
                      : g == REPETITION ? "Repetition"
                      : g == FIFTY_MOVE ? "Fifty-move rule" : "?") << " "
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

    for (int id = 0; id < threads; id++) {
        matches.push_back(new Match(engine1_path, engine2_path, time, id, fen_file));
        thread_pool.emplace_back(&Match::run, matches.back(), &status);
    }

    for (std::thread& thread : thread_pool)
        thread.join();

    int e1_wins = 0, e2_wins = 0, draws = 0;

    for (Match *m : matches) {
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
