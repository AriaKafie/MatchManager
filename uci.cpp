
#include "uci.h"

Move uci_to_move(const std::string& uci, Position& pos)
{
    for (Move list[MAX_MOVES], *m = list, *end = pos.get_moves(list); m != end; m++)
        if (move_to_uci(*m) == uci) return *m;

    return NULLMOVE;
}