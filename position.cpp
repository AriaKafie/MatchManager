
#include "position.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>

#include "bitboard.h"
#include "uci.h"

constexpr std::string_view piece_to_char = "  PNBRQK  pnbrqk";

namespace Zobrist
{
    static uint64_t Side = 0xeeb3b2fe864d41e5ull;
    static uint64_t hash[B_KING + 1][SQUARE_NB];
    static uint64_t enpassant[SQUARE_NB];
    static uint64_t castling[1 << 4];
}

uint64_t Position::hash() const
{
    uint64_t key = side_to_move() == WHITE ? 0 : Zobrist::Side;

    for (Square sq = H1; sq <= A8; sq++)
        key ^= Zobrist::hash[piece_on(sq)][sq];
    
    return key ^ Zobrist::castling[state_info.castling_rights]
               ^ Zobrist::enpassant[state_info.ep_sq];
}

void Position::init()
{
    std::mt19937_64 rng(221564671644);

    memset(Zobrist::hash, 0, sizeof(Zobrist::hash));

    for (Piece pc : { W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
                      B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING })
    {
        for (Square sq = H1; sq <= A8; sq++)
            Zobrist::hash[pc][sq] = rng();
    }

    for (uint8_t castling_rights = 0; castling_rights <= 0b1111; castling_rights++)
        Zobrist::castling[castling_rights] = rng();

    Zobrist::enpassant[0] = 0ull;

    for (Square s = G1; s <= A8; Zobrist::enpassant[s++] = rng());
}

GameState Position::game_state()
{
    if (state_info.halfmove_clock >= 100)
        return DRAW;

    for (int i = history.size() - 2, occurrences = 1; i >= 0; i--)
    {
        if (history[i] == history.back()) occurrences++;

        if (occurrences == 3) return DRAW;
    }       
    
    if (Move list[MAX_MOVES], *end = get_moves(list); list == end)
    {
        Color us = side_to_move(), them = !us;
        Square ksq = lsb(bb(make_piece(us, KING)));

        Bitboard checkers = PawnAttacks[us][ksq]            &  bb(make_piece(them, PAWN))
                          | knight_attacks(ksq)             &  bb(make_piece(them, KNIGHT))
                          | bishop_attacks(ksq, occupied()) & (bb(make_piece(them, QUEEN)) | bb(make_piece(them, BISHOP)))
                          | rook_attacks(ksq, occupied())   & (bb(make_piece(them, QUEEN)) | bb(make_piece(them, ROOK)));

        return checkers ? MATE : DRAW;
    }

    return ONGOING;
}

void Position::do_move(Move m)
{
    Color us = state_info.side_to_move, them = !us;

    Piece Pawn  = make_piece(us, PAWN);
    Piece Rook  = make_piece(us, ROOK);
    Piece Queen = make_piece(us, QUEEN);
    Piece King  = make_piece(us, KING);

    Direction Up  = us == WHITE ? NORTH : SOUTH;
    Direction Up2 = Up * 2;

    Square from = from_sq(m);
    Square to   = to_sq(m);

    if (piece_type_on(from) == PAWN || piece_on(to))
        state_info.halfmove_clock = 0;
    else
        state_info.halfmove_clock++;

    state_info.ep_sq = (from + Up) * !(to - from ^ Up2 | piece_type_on(from) ^ PAWN);
    state_info.side_to_move ^= 1;

    Bitboard zero_to = ~square_bb(to);
    Bitboard from_to =  square_bb(from, to);

    switch (type_of(m))
    {
    case NORMAL:
        bitboards[board[to]] &= zero_to;
        bitboards[them] &= zero_to;
        bitboards[board[from]] ^= from_to;
        bitboards[us] ^= from_to;

        board[to] = board[from];
        board[from] = NO_PIECE;

        update_castling_rights(us);
        
        history.push_back(hash());
        return;
    case PROMOTION:
    {
        Piece promotion = make_piece(us, promotion_type(m));
        
        bitboards[board[to]] &= zero_to;
        bitboards[them] &= zero_to;
        bitboards[Pawn] ^= square_bb(from);
        bitboards[promotion] ^= ~zero_to;
        bitboards[us] ^= from_to;

        board[to] = promotion;
        board[from] = NO_PIECE;
        
        update_castling_rights(us);
        
        history.push_back(hash());
        return;
    }
    case CASTLING:
    {
        Move rook_move = to == G1 ? make_move(H1, F1)
                       : to == C1 ? make_move(A1, D1)
                       : to == G8 ? make_move(H8, F8) : make_move(A8, D8);

        Square rook_from = from_sq(rook_move), rook_to = to_sq(rook_move);        
        Bitboard rook_from_to = square_bb(rook_from, rook_to);

        bitboards[King] ^= from_to;
        bitboards[Rook] ^= rook_from_to;
        bitboards[us] ^= from_to ^ rook_from_to;

        board[from] = NO_PIECE;
        board[rook_from] = NO_PIECE;
        board[to] = King;
        board[rook_to] = Rook;

        update_castling_rights(us);

        history.push_back(hash());
        return;
    }
    case ENPASSANT:
        Piece EnemyPawn = make_piece(them, PAWN);

        Square capsq = to + (us == WHITE ? SOUTH : NORTH);

        bitboards[Pawn] ^= from_to;
        bitboards[EnemyPawn] ^= square_bb(capsq);
        bitboards[us] ^= from_to;
        bitboards[them] ^= square_bb(capsq);

        board[from] = NO_PIECE;
        board[to] = Pawn;
        board[capsq] = NO_PIECE;

        history.push_back(hash());
        return;
    }
}

void Position::update_castling_rights(Color just_moved)
{
    Bitboard mask = just_moved == WHITE ? square_bb(A1, E1, H1, A8, H8) : square_bb(A8, E8, H8, A1, H1);
    state_info.castling_rights &= castle_masks[just_moved][pext(bitboards[just_moved], mask)];
}

void Position::set(const std::string& fen)
{
    memset(board, NO_PIECE, sizeof(board));
    memset(bitboards, 0ull, sizeof(bitboards));

    Square             sq = A8;
    std::istringstream is(fen);
    std::string        pieces, color, castling, enpassant;

    is >> pieces >> color >> castling >> enpassant;

    for (char token : pieces)
    {
        if (std::isdigit(token))
            sq -= token - '0'; 
        else if (size_t piece = piece_to_char.find(token); piece != std::string::npos)
        {
            board[sq] = piece;
            bitboards[piece] |= square_bb(sq);
            bitboards[color_of(piece)] |= square_bb(sq);
            sq--;
        }
    }

    state_info.side_to_move = color == "w" ? WHITE : BLACK;

    state_info.castling_rights = state_info.ep_sq = 0;

    for (char token : castling)
        if (size_t idx = std::string("qkQK").find(token); idx != std::string::npos)
            state_info.castling_rights |= 1 << idx;

    if (enpassant != "-")
        state_info.ep_sq = uci_to_square(enpassant);

    state_info.halfmove_clock = 0;

    history.clear();
    history.push_back(hash());
}

std::string Position::to_string() const
{
    std::stringstream ss;

    ss << "\n+---+---+---+---+---+---+---+---+\n";

    for (Square sq = A8; sq >= H1; sq--)
    {
        ss << "| " << piece_to_char[board[sq]] << " ";

        if (sq % 8 == 0)
            ss << "| " << (sq / 8 + 1) << "\n+---+---+---+---+---+---+---+---+\n";
    }

    ss << "  a   b   c   d   e   f   g   h\n\nFen: " << fen() << "\nKey: " << std::setw(16) << std::setfill('0') << std::hex << std::uppercase << hash() << "\n";

    return ss.str();
}

std::string Position::fen() const
{
    std::stringstream fen;

    for (int rank = 7; rank >= 0; rank--)
    {
        for (int file = 7; file >= 0; file--)
        {
            if (Piece pc = piece_on(rank * 8 + file))
                fen << piece_to_char[pc];
            else
            {
                int empty = 0, f;

                for (f = file; f >= 0 && !piece_on(rank * 8 + f); f--)
                    empty++;

                fen << empty;

                file = f + 1;
            }
        }

        if (rank)
            fen << "/";
    }

    fen << " " << "wb"[state_info.side_to_move] << " ";

    if (!state_info.castling_rights)
        fen << "-";
    else
    {
        if (kingside_rights (WHITE)) fen << "K";
        if (queenside_rights(WHITE)) fen << "Q";
        if (kingside_rights (BLACK)) fen << "k";
        if (queenside_rights(BLACK)) fen << "q";
    }
  
    fen << " " << (state_info.ep_sq ? square_to_uci(state_info.ep_sq) : "-") << " " << (int)state_info.halfmove_clock;

    return fen.str();
}