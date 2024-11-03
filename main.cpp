
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>

#include "bitboard.h"
#include "engine.h"
#include "position.h"
#include "uci.h"

int main()
{
    Bitboards::init();

    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    std::string tokens, token;
    do
    {
        std::getline(std::cin, tokens);
        
        std::istringstream is(tokens);
        is >> token;

        if (token == "d")
        {
            std::cout << pos.to_string() << std::endl;
        }
        else if (token == "moves")
        {
            for (Move m; is >> token && (m=uci_to_move(token, pos)); pos.do_move(m));
        }
        else if (token == "position")
        {
            std::string pieces, color, castling, enpassant;
            is >> pieces >> color >> castling >> enpassant;
            pos.set(pieces + " " + color + " " + castling + " " + enpassant);
        }

    } while(tokens != "quit");
}