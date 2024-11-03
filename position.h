
#ifndef POSITION_H
#define POSITION_H

#include <string>

#include "bitboard.h"
#include "types.h"

constexpr int MAX_MOVES = 128;

enum GameState { ONGOING, MATE, DRAW };

struct StateInfo
{
    Square  ep_sq;
    uint8_t castling_rights;
    Color   side_to_move;
    uint8_t halfmove_clock;
};

class Position
{
public:
    Position() { set(); }

    Move *get_moves(Move *list);

    void set(const std::string& fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ");
    void do_move(Move m);
    void update_castling_rights(Color just_moved);
    std::string fen() const;
    std::string to_string() const;
    GameState game_state();

    bool kingside_rights  (Color Perspective) const { return state_info.castling_rights & (Perspective == WHITE ? 0b1000 : 0b0010); }
    bool queenside_rights (Color Perspective) const { return state_info.castling_rights & (Perspective == WHITE ? 0b0100 : 0b0001); }
    bool white_to_move() const { return state_info.side_to_move == WHITE; }

    Color side_to_move() const { return state_info.side_to_move; }

    Piece piece_on(Square sq) const { return board[sq]; }

    PieceType piece_type_on(Square sq) const { return type_of(board[sq]); }

    Bitboard bb(Piece pc) const { return bitboards[pc]; }

    Bitboard occupied() const { return bitboards[WHITE] | bitboards[BLACK]; }

    Bitboard ep_bb() const { return square_bb(state_info.ep_sq); }

    Square ep_sq() const { return state_info.ep_sq; }
    
private:
    Bitboard bitboards[16];
    Piece board[SQUARE_NB];

    StateInfo state_info;
};

#endif