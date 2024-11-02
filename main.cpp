
#include <iostream>
#include <chrono>
#include <thread>

#include "engine.h"

int main()
{
    Engine e("C:\\Users\\14244\\source\\repos\\search_root\\x64\\Release\\search_root.exe", 500);

    std::string s = e.best_move();

    std::cout << s << std::endl;
}