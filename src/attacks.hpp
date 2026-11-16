#pragma once
#include "board.hpp"

constexpr uint64_t NOT_A_FILE  = 18374403900871474942ULL;
constexpr uint64_t NOT_H_FILE  = 9187201950435737471ULL;
constexpr uint64_t NOT_HG_FILE = 4557430888798830399ULL;
constexpr uint64_t NOT_AB_FILE = 18229723555195321596ULL;

uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];

inline void init_leaper_attacks() {
    for (int square = 0; square < 64; square++) {
        uint64_t bitboard = 1ULL << square;

        // Pawn attacks
        pawn_attacks[White][square] = ((bitboard << 7) & NOT_H_FILE) | ((bitboard << 9) & NOT_A_FILE);
        pawn_attacks[Black][square] = ((bitboard >> 7) & NOT_A_FILE) | ((bitboard >> 9) & NOT_H_FILE);

        // Knight attacks
        knight_attacks[square] = ((bitboard << 17) & NOT_A_FILE)  | ((bitboard << 15) & NOT_H_FILE)  |
                                 ((bitboard << 10) & NOT_AB_FILE) | ((bitboard << 6)  & NOT_HG_FILE) |
                                 ((bitboard >> 17) & NOT_H_FILE)  | ((bitboard >> 15) & NOT_A_FILE)  |
                                 ((bitboard >> 10) & NOT_HG_FILE) | ((bitboard >> 6)  & NOT_AB_FILE);

        // King attacks (all 8 directions)
        king_attacks[square] = (bitboard << 8) | (bitboard >> 8) |
                               ((bitboard << 1) & NOT_A_FILE) | ((bitboard << 9) & NOT_A_FILE) | ((bitboard >> 7) & NOT_A_FILE) |
                               ((bitboard >> 1) & NOT_H_FILE) | ((bitboard >> 9) & NOT_H_FILE) | ((bitboard << 7) & NOT_H_FILE);
    }
}

inline uint64_t get_rook_attacks(int square, uint64_t occupancy) {
    uint64_t attacks = 0ULL;
    int target_rank = square / 8;
    int target_file = square % 8;

    for (int r = target_rank + 1; r < 8; r++) {
        int sq = r * 8 + target_file;
        set_bit(attacks, sq);
        if (get_bit(occupancy, sq)) break;
    }
    for (int r = target_rank - 1; r >= 0; r--) {
        int sq = r * 8 + target_file;
        set_bit(attacks, sq);
        if (get_bit(occupancy, sq)) break;
    }
    for (int f = target_file + 1; f < 8; f++) {
        int sq = target_rank * 8 + f;
        set_bit(attacks, sq);
        if (get_bit(occupancy, sq)) break;
    }
    for (int f = target_file - 1; f >= 0; f--) {
        int sq = target_rank * 8 + f;
        set_bit(attacks, sq);
        if (get_bit(occupancy, sq)) break;
    }
    return attacks;
}

inline uint64_t get_bishop_attacks(int square, uint64_t occupancy) {
    uint64_t attacks = 0ULL;
    int target_rank = square / 8;
    int target_file = square % 8;

    for (int r = target_rank + 1, f = target_file + 1; r < 8 && f < 8; r++, f++) {
        int sq = r * 8 + f;
        set_bit(attacks, sq);
        if (get_bit(occupancy, sq)) break;
    }
    for (int r = target_rank + 1, f = target_file - 1; r < 8 && f >= 0; r++, f--) {
        int sq = r * 8 + f;
        set_bit(attacks, sq);
        if (get_bit(occupancy, sq)) break;
    }
    for (int r = target_rank - 1, f = target_file + 1; r >= 0 && f < 8; r--, f++) {
        int sq = r * 8 + f;
        set_bit(attacks, sq);
        if (get_bit(occupancy, sq)) break;
    }
    for (int r = target_rank - 1, f = target_file - 1; r >= 0 && f >= 0; r--, f--) {
        int sq = r * 8 + f;
        set_bit(attacks, sq);
        if (get_bit(occupancy, sq)) break;
    }
    return attacks;
}

inline uint64_t get_queen_attacks(int square, uint64_t occupancy) {
    return get_rook_attacks(square, occupancy) | get_bishop_attacks(square, occupancy);
}

inline bool is_square_attacked(int square, int by_side, const Board& board) {
    if (by_side == White && (pawn_attacks[Black][square] & board.bitboards[P])) return true;
    if (by_side == Black && (pawn_attacks[White][square] & board.bitboards[p])) return true;

    uint64_t knights = (by_side == White) ? board.bitboards[N] : board.bitboards[n];
    if (knight_attacks[square] & knights) return true;

    uint64_t kings = (by_side == White) ? board.bitboards[K] : board.bitboards[k];
    if (king_attacks[square] & kings) return true;

    uint64_t bishops_queens = (by_side == White)
        ? (board.bitboards[B] | board.bitboards[Q])
        : (board.bitboards[b] | board.bitboards[q]);
    if (get_bishop_attacks(square, board.occupancies[Both]) & bishops_queens) return true;

    uint64_t rooks_queens = (by_side == White)
        ? (board.bitboards[R] | board.bitboards[Q])
        : (board.bitboards[r] | board.bitboards[q]);
    if (get_rook_attacks(square, board.occupancies[Both]) & rooks_queens) return true;

    return false;
}