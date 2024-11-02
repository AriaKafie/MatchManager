
#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <windows.h>

class Engine
{
public:
    Engine(const char *exe, int thinktime);
   ~Engine() { write_to_stdin("stop\nquit\n"); }

    void write_to_stdin(const std::string& message);
    std::string board_string();
    std::string read_stdout();
    std::string best_move();

private:
    int    m_thinktime;
    HANDLE m_stdin;
    HANDLE m_stdout;
    HANDLE m_process;
};

#endif