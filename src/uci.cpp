
#include "uci.h"

#include <algorithm>
#include <sstream>

Move uci_to_move(const std::string& uci, const Position& pos)
{
    for (Move list[MAX_MOVES], *m = list, *end = pos.get_moves(list); m != end; m++)
        if (move_to_uci(*m) == uci) return *m;

    return Move::null();
}

std::string move_to_san(Move m, Position pos)
{
    char pt2c[] = "  PNBRQK";
    char f2c [] = "hgfedcba";
    char r2c [] = "12345678";

    Square    from    = m.from_sq();
    Square    to      = m.to_sq();
    Piece     pc      = pos.piece_on(from);
    PieceType pt      = type_of(pc);
    bool      capture = pos.piece_on(to) || m.type_of() == ENPASSANT;
    
    std::stringstream san;

    if (m.type_of() == CASTLING)
    {
        san << (file_of(to) == FILE_G_ENUM ? "O-O" : "O-O-O");
    }
    else if (pt == PAWN)
    {
        if (capture)
            san << f2c[file_of(from)] << "x";

        san << square_to_uci(to);

        if (m.type_of() == PROMOTION)
            san << "=" << pt2c[m.promotion_type()];
    }
    else
    {
        int i;
        std::vector<std::string> notations[4];

        for (Move list[MAX_MOVES], *mp = list, *end = pos.get_moves(list); mp != end; mp++)
        {
            if (*mp == m) i = mp - list;

            Square f = mp->from_sq();
            Square t = mp->to_sq();

            std::string left = std::string(1, pt2c[type_of(pos.piece_on(f))]);
            std::string right = pos.piece_on(t)
                ? std::string("x") + square_to_uci(t)
                : square_to_uci(t);

            char file = f2c[file_of(f)];
            char rank = r2c[rank_of(f)];

            notations[0].push_back(left + right);
            notations[1].push_back(left + file + right);
            notations[2].push_back(left + rank + right);
            notations[3].push_back(left + file + rank + right);
        }

        for (const std::vector<std::string>& v : notations)
            if (std::count(v.begin(), v.end(), v[i]) == 1) { san << v[i]; break; }
    }
    
    pos.do_move(m);

    if (pos.game_state() == MATE) {
        san << '#';
    } else if (pos.checkers()) {
        san << '+';
    }

    return san.str();
}