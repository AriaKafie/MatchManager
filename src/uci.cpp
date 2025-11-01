
#include "uci.h"

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
        san << pt2c[pt];

        bool needs_disambig = false;
        bool needs_file_disambig = false;
        bool needs_rank_disambig = false;

        for (Move list[128], *mp = list, *end = pos.get_moves(list); mp != end; mp++)
        {
            Square f = mp->from_sq();
            Square t = mp->to_sq();

            if (*mp != m && pos.piece_on(f) == pc && t == to)
            {
                needs_disambig = true;

                if (rank_of(f) == rank_of(from))
                    needs_file_disambig = true;

                if (file_of(f) == file_of(from))
                    needs_rank_disambig = true;
            }
        }

        if (needs_disambig && !needs_file_disambig && !needs_rank_disambig)
            san << f2c[file_of(from)];

        if (needs_file_disambig)
            san << f2c[file_of(from)];

        if (needs_rank_disambig)
            san << r2c[rank_of(from)];

        if (capture)
            san << "x";

        san << square_to_uci(to);
    }
    
    pos.do_move(m);

    if (pos.game_state() == MATE) {
        san << '#';
    } else if (pos.checkers()) {
        san << '+';
    }

    return san.str();
}