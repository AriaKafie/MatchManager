
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint64_t Bitboard;
typedef uint16_t Move;
typedef uint16_t MoveType;
typedef uint8_t  Piece;
typedef uint8_t  PieceType;
typedef uint8_t  Color;
typedef int8_t   Direction;
typedef int8_t   Square;

constexpr Move NO_MOVE = 0;
constexpr Move NULL_MOVE = 0;

enum { WHITE, BLACK, COLOR_NB = 2 };

enum
{
    NO_PIECE,
      PAWN =          2,   KNIGHT,   BISHOP,   ROOK,   QUEEN,   KING,
    W_PAWN =       PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = W_PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
};

enum {
    H1, G1, F1, E1, D1, C1, B1, A1,
    H2, G2, F2, E2, D2, C2, B2, A2,
    H3, G3, F3, E3, D3, C3, B3, A3,
    H4, G4, F4, E4, D4, C4, B4, A4,
    H5, G5, F5, E5, D5, C5, B5, A5,
    H6, G6, F6, E6, D6, C6, B6, A6,
    H7, G7, F7, E7, D7, C7, B7, A7,
    H8, G8, F8, E8, D8, C8, B8, A8,
    SQUARE_NB = 64
};

enum {
    NORMAL           = 0,
    PROMOTION        = 1 << 12,
    ENPASSANT        = 2 << 12,
    CASTLING         = 3 << 12,
    KNIGHT_PROMOTION = PROMOTION + ((KNIGHT - KNIGHT) << 14),
    BISHOP_PROMOTION = PROMOTION + ((BISHOP - KNIGHT) << 14),
    ROOK_PROMOTION   = PROMOTION + ((ROOK - KNIGHT) << 14),
    QUEEN_PROMOTION  = PROMOTION + ((QUEEN - KNIGHT) << 14)
};

enum {
    NORTH      = 8,
    EAST       = -1,
    SOUTH      = -8,
    WEST       = 1,
    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

constexpr PieceType type_of(Piece p) {
    return p & 7;
}

inline Move make_move(Square from, Square to) {
    return (from << 6) + to;
}

template<MoveType T>
constexpr Move make_move(Square from, Square to) {
    return T + (from << 6) + to;
}

inline Square from_sq(Move m) {
    return (m >> 6) & 0x3f;
}

inline Square to_sq(Move m) {
    return m & 0x3f;
}

inline MoveType type_of(Move m) {
    return m & 0x3000;
}

inline PieceType promotion_type(Move m) {
    return (m >> 14) + KNIGHT;
}

constexpr Piece make_piece(Color c, PieceType pt) {
    return pt + (c << 3);
}

inline Color color_of(Piece p) {
    return p >> 3;
}

inline bool is_ok(Square s) {
    return s >= H1 && s <= A8;
}

#endif