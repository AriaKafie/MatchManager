
#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <windows.h>

class Engine
{
public:
    Engine(const std::string &path, int thinktime);
   ~Engine() { kill(); }

    void write_to_stdin(const std::string& message);
    void kill() { write_to_stdin("stop\nquit\n"); }
    std::string read_stdout();
    std::string best_move();
    std::string name() const { return m_name; }

    int wins;

private:
    int    m_thinktime;
    HANDLE m_stdin;
    HANDLE m_stdout;

    std::string m_name;
};

#endif