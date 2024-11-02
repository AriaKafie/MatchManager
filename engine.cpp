
#include "engine.h"

#include <iostream>
#include <cctype>
#include <chrono>
#include <thread>

static std::string strip(const std::string& input)
{
    size_t first = input.find_first_not_of(" \t\n\r\f\v");

    if (first == std::string::npos)
        return "";

    size_t last = input.find_last_not_of(" \t\n\r\f\v");
    return input.substr(first, last - first + 1);
}

std::string Engine::best_move()
{
    write_to_stdin("go movetime " + std::to_string(m_thinktime) + "\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(m_thinktime));

    for (std::string std_out;; std_out = read_stdout())
        if (size_t s = std_out.find("bestmove"); s != std::string::npos) return strip(std_out.substr(s + 9));
}

std::string Engine::board_string()
{
    write_to_stdin("d\n");

    for (std::string board;; board = read_stdout())
        if (!board.empty()) return board;
}

void Engine::write_to_stdin(const std::string& message)
{
    DWORD written;
    WriteFile(m_stdin, message.c_str(), message.size(), &written, NULL);
    FlushFileBuffers(m_stdin);
}

std::string Engine::read_stdout()
{
    const int bufferSize = 4096;
    char      buffer[bufferSize];
    DWORD     read;

    if (!ReadFile(m_stdout, buffer, bufferSize - 1, &read, NULL) || read == 0)
        return "";

    buffer[read] = '\0';

    return strip(std::string(buffer));
}

Engine::Engine(const char *exe, int thinktime) : m_stdin    (NULL),
                                                 m_stdout   (NULL),
                                                 m_thinktime(thinktime)
{
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    SECURITY_ATTRIBUTES saAttr;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStdoutWr, hChildStdinRd;

    // Create pipes for the child's STDOUT and STDIN
    if (!CreatePipe(&m_stdout, &hChildStdoutWr, &saAttr, 0)     ||  // stdout pipe
        !SetHandleInformation(m_stdout, HANDLE_FLAG_INHERIT, 0) ||  // Make read handle non-inheritable
        !CreatePipe(&hChildStdinRd, &m_stdin, &saAttr, 0)       ||  // stdin pipe
        !SetHandleInformation(m_stdin, HANDLE_FLAG_INHERIT, 0))
    {
        std::cerr << "Error creating pipes." << std::endl;
        std::exit(1);
    }

    // Configure the STARTUPINFO structure for redirection
    siStartInfo.hStdError = hChildStdoutWr;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.hStdInput = hChildStdinRd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    int len = strlen(exe) + 1;
    LPWSTR wexe = new WCHAR[len];
    MultiByteToWideChar(CP_ACP, 0, exe, -1, wexe, len);

    // Create the child process
    if (!CreateProcess(NULL, wexe, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo))
    {
        std::cerr << "CreateProcess failed." << std::endl;
        std::exit(1);
    }

    delete[] wexe;

    m_process = piProcInfo.hProcess;

    // Close unneeded handles
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    CloseHandle(hChildStdoutWr);
    CloseHandle(hChildStdinRd);
}
