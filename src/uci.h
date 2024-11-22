
#ifndef UCI_H
#define UCI_H

#include <string>

#include "position.h"
#include "types.h"

inline Square uci_to_square(const std::string& uci) {
    return 8 * (uci[1] - '1') + 'h' - uci[0];
}

inline std::string square_to_uci(Square sq) {
    return std::string(1, "hgfedcba"[sq % 8]) + std::string(1, "12345678"[sq / 8]);
}

inline std::string move_to_uci(Move m)
{
    return type_of(m) == PROMOTION ? square_to_uci(from_sq(m)) + square_to_uci(to_sq(m)) + "   nbrq"[promotion_type(m)]
                                   : square_to_uci(from_sq(m)) + square_to_uci(to_sq(m));
}

std::string move_to_san(Move m, Position& pos);

#endif