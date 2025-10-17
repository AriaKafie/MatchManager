
#include <iostream>
#include <regex>
#include <string>

void help() {
    std::cout << "Usage: MatchManager --flag=value" << R"(
Required flags:
--engine1         path to engine1
--engine2         path to engine2

Optional flags:
--time            milliseconds of movetime [100]
--threads         # of matches to run in parallel [1]
--fen_file        path to file with starting positions [lc01k.txt]
)";
    std::exit(1);
}

void verify_args(int argc, char *argv[])
{
    const std::string patterns[] = {
        "--engine[12]=.+",
        "--time=[1-9]\\d*",
        "--threads=[1-9]\\d*",
        "--fen_file=.+"
    };

    for (int i = 1; i < argc; i++)
    {
        bool invalid = true;

        for (const std::string& pattern : patterns)
        {
            if (std::regex_match(argv[i], std::regex(pattern)))
            {
                invalid = false;
                break;
            }
        }

        if (invalid)
        {
            std::cout << "Bad arg '" << argv[i] << "'" << std::endl;
            help();
        }
    }
}

std::string get_with_default(const std::string& flag, int argc, char *argv[], std::string defval)
{
    std::string prefix = "--" + flag + "=";

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg.find(prefix) == 0)
        {
            std::string val = arg.substr(prefix.length());
            std::cout << "found flag " << flag << " = " << val << std::endl;

            return val;
        }
    }

    return defval;
}

std::string get_required(const std::string& flag, int argc, char* argv[])
{
    std::string val = get_with_default(flag, argc, argv, "");

    if (val.empty()) {
        std::cout << "Required flag " << flag << " was not found" << std::endl;
        help();
    }

    return val;
}