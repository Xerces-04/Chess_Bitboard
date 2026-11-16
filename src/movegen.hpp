#pragma once
#include "board.hpp"
#include "attacks.hpp"

// Bit scan helpers (GCC/Clang built-ins; if on MSVC, use _BitScanForward64)
inline int get_lsb_index(uint64_t bb) {
    return bb ? __builtin_ctzll(bb) : -1;
}

// ==========================================
// 1. MOVE ENCODING (Packed into 32-bit int)
// ==========================================
// Bits 0..5:   Source square (0..63)
// Bits 6..11:  Target square (0..63)
// Bits 12..15: Moving piece (P..k)
// Bits 16..19: Promoted piece (P..k, or NO_PIECE = 12)
// Bit  20:     Capture flag
// Bit  21:     Double pawn push flag
// Bit  22:     En passant flag
// Bit  23:     Castling flag

using Move = uint32_t;

inline Move encode_move(int source, int target, int piece, int promoted,
                        bool capture, bool double_push, bool ep, bool castling) {
    return (source) |
           (target << 6) |
           (piece << 12) |
           (promoted << 16) |
           (capture << 20) |
           (double_push << 21) |
           (ep << 22) |
           (castling << 23);
}

inline int get_move_source(Move m)     { return m & 0x3F; }
inline int get_move_target(Move m)     { return (m >> 6) & 0x3F; }
inline int get_move_piece(Move m)      { return (m >> 12) & 0xF; }
inline int get_move_promoted(Move m)   { return (m >> 16) & 0xF; }
inline bool is_move_capture(Move m)    { return (m >> 20) & 1; }
inline bool is_move_double(Move m)     { return (m >> 21) & 1; }
inline bool is_move_ep(Move m)         { return (m >> 22) & 1; }
inline bool is_move_castling(Move m)   { return (m >> 23) & 1; }

struct MoveList {
    Move moves[256];
    int count = 0;

    void add(Move m) {
        moves[count++] = m;
    }
};

// Castling rights update table indexed by square (0..63)
// Whenever any piece moves from or to a square, `castle &= castling_rights[sq]`
constexpr int castling_rights_mask[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11
};

// ==========================================
// 2. PSEUDO-LEGAL MOVE GENERATION
// ==========================================
inline void generate_moves(const Board& board, MoveList& move_list) {
    move_list.count = 0;
    int side = board.side;
    int enemy = (side == White) ? Black : White;

    uint64_t friendly_occ = board.occupancies[side];
    uint64_t enemy_occ    = board.occupancies[enemy];
    uint64_t all_occ      = board.occupancies[Both];

    // --- PAWNS & CASTLING (Direction-dependent) ---
    if (side == White) {
        uint64_t pawns = board.bitboards[P];
        while (pawns) {
            int src = get_lsb_index(pawns);
            int tgt = src + 8;

            // Quiet pushes
            if (tgt < 64 && !get_bit(all_occ, tgt)) {
                if (src >= 48 && src <= 55) { // Rank 7 -> Promotion
                    move_list.add(encode_move(src, tgt, P, Q, false, false, false, false));
                    move_list.add(encode_move(src, tgt, P, R, false, false, false, false));
                    move_list.add(encode_move(src, tgt, P, B, false, false, false, false));
                    move_list.add(encode_move(src, tgt, P, N, false, false, false, false));
                } else {
                    move_list.add(encode_move(src, tgt, P, NO_PIECE, false, false, false, false));
                    // Double push from Rank 2 (squares 8..15)
                    if (src >= 8 && src <= 15 && !get_bit(all_occ, src + 16)) {
                        move_list.add(encode_move(src, src + 16, P, NO_PIECE, false, true, false, false));
                    }
                }
            }

            // Captures
            uint64_t attacks = pawn_attacks[White][src] & enemy_occ;
            while (attacks) {
                int cap_tgt = get_lsb_index(attacks);
                if (src >= 48 && src <= 55) {
                    move_list.add(encode_move(src, cap_tgt, P, Q, true, false, false, false));
                    move_list.add(encode_move(src, cap_tgt, P, R, true, false, false, false));
                    move_list.add(encode_move(src, cap_tgt, P, B, true, false, false, false));
                    move_list.add(encode_move(src, cap_tgt, P, N, true, false, false, false));
                } else {
                    move_list.add(encode_move(src, cap_tgt, P, NO_PIECE, true, false, false, false));
                }
                pop_bit(attacks, cap_tgt);
            }

            // En Passant
            if (board.en_passant != -1 && (pawn_attacks[White][src] & (1ULL << board.en_passant))) {
                move_list.add(encode_move(src, board.en_passant, P, NO_PIECE, true, false, true, false));
            }

            pop_bit(pawns, src);
        }

        // White Castling (e1=4, f1=5, g1=6, d1=3, c1=2, b1=1)
        if (board.castle & WK) {
            if (!get_bit(all_occ, 5) && !get_bit(all_occ, 6)) {
                if (!is_square_attacked(4, Black, board) && !is_square_attacked(5, Black, board)) {
                    move_list.add(encode_move(4, 6, K, NO_PIECE, false, false, false, true));
                }
            }
        }
        if (board.castle & WQ) {
            if (!get_bit(all_occ, 3) && !get_bit(all_occ, 2) && !get_bit(all_occ, 1)) {
                if (!is_square_attacked(4, Black, board) && !is_square_attacked(3, Black, board)) {
                    move_list.add(encode_move(4, 2, K, NO_PIECE, false, false, false, true));
                }
            }
        }
    } else {
        uint64_t pawns = board.bitboards[p];
        while (pawns) {
            int src = get_lsb_index(pawns);
            int tgt = src - 8;

            // Quiet pushes
            if (tgt >= 0 && !get_bit(all_occ, tgt)) {
                if (src >= 8 && src <= 15) { // Rank 2 -> Promotion
                    move_list.add(encode_move(src, tgt, p, q, false, false, false, false));
                    move_list.add(encode_move(src, tgt, p, r, false, false, false, false));
                    move_list.add(encode_move(src, tgt, p, b, false, false, false, false));
                    move_list.add(encode_move(src, tgt, p, n, false, false, false, false));
                } else {
                    move_list.add(encode_move(src, tgt, p, NO_PIECE, false, false, false, false));
                    // Double push from Rank 7 (squares 48..55)
                    if (src >= 48 && src <= 55 && !get_bit(all_occ, src - 16)) {
                        move_list.add(encode_move(src, src - 16, p, NO_PIECE, false, true, false, false));
                    }
                }
            }

            // Captures
            uint64_t attacks = pawn_attacks[Black][src] & enemy_occ;
            while (attacks) {
                int cap_tgt = get_lsb_index(attacks);
                if (src >= 8 && src <= 15) {
                    move_list.add(encode_move(src, cap_tgt, p, q, true, false, false, false));
                    move_list.add(encode_move(src, cap_tgt, p, r, true, false, false, false));
                    move_list.add(encode_move(src, cap_tgt, p, b, true, false, false, false));
                    move_list.add(encode_move(src, cap_tgt, p, n, true, false, false, false));
                } else {
                    move_list.add(encode_move(src, cap_tgt, p, NO_PIECE, true, false, false, false));
                }
                pop_bit(attacks, cap_tgt);
            }

            // En Passant
            if (board.en_passant != -1 && (pawn_attacks[Black][src] & (1ULL << board.en_passant))) {
                move_list.add(encode_move(src, board.en_passant, p, NO_PIECE, true, false, true, false));
            }

            pop_bit(pawns, src);
        }

        // Black Castling (e8=60, f8=61, g8=62, d8=59, c8=58, b8=57)
        if (board.castle & BK) {
            if (!get_bit(all_occ, 61) && !get_bit(all_occ, 62)) {
                if (!is_square_attacked(60, White, board) && !is_square_attacked(61, White, board)) {
                    move_list.add(encode_move(60, 62, k, NO_PIECE, false, false, false, true));
                }
            }
        }
        if (board.castle & BQ) {
            if (!get_bit(all_occ, 59) && !get_bit(all_occ, 58) && !get_bit(all_occ, 57)) {
                if (!is_square_attacked(60, White, board) && !is_square_attacked(59, White, board)) {
                    move_list.add(encode_move(60, 58, k, NO_PIECE, false, false, false, true));
                }
            }
        }
    }

    // --- KNIGHTS, BISHOPS, ROOKS, QUEENS, KINGS ---
    int start_piece = (side == White) ? N : n;
    int end_piece   = (side == White) ? K : k;

    for (int piece = start_piece; piece <= end_piece; piece++) {
        uint64_t bb = board.bitboards[piece];
        while (bb) {
            int src = get_lsb_index(bb);
            uint64_t attacks = 0ULL;

            if (piece == N || piece == n)      attacks = knight_attacks[src];
            else if (piece == B || piece == b) attacks = get_bishop_attacks(src, all_occ);
            else if (piece == R || piece == r) attacks = get_rook_attacks(src, all_occ);
            else if (piece == Q || piece == q) attacks = get_queen_attacks(src, all_occ);
            else if (piece == K || piece == k) attacks = king_attacks[src];

            // Mask out friendly pieces
            attacks &= ~friendly_occ;

            while (attacks) {
                int tgt = get_lsb_index(attacks);
                bool is_cap = get_bit(enemy_occ, tgt);
                move_list.add(encode_move(src, tgt, piece, NO_PIECE, is_cap, false, false, false));
                pop_bit(attacks, tgt);
            }
            pop_bit(bb, src);
        }
    }
}

// ==========================================
// 3. MAKE MOVE (Returns false if King is in check)
// ==========================================
inline bool make_move(Board& board, Move move) {
    Board backup = board; // Fast copy-restore if move turns out illegal

    int src      = get_move_source(move);
    int tgt      = get_move_target(move);
    int piece    = get_move_piece(move);
    int promoted = get_move_promoted(move);
    bool capture = is_move_capture(move);
    bool dbl     = is_move_double(move);
    bool ep      = is_move_ep(move);
    bool castle  = is_move_castling(move);

    // 1. Move the piece
    pop_bit(board.bitboards[piece], src);
    set_bit(board.bitboards[piece], tgt);

    // 2. Handle captures (remove enemy piece on target square)
    if (capture && !ep) {
        int enemy_start = (board.side == White) ? p : P;
        int enemy_end   = (board.side == White) ? k : K;
        for (int bb_idx = enemy_start; bb_idx <= enemy_end; bb_idx++) {
            if (get_bit(board.bitboards[bb_idx], tgt)) {
                pop_bit(board.bitboards[bb_idx], tgt);
                break;
            }
        }
    }

    // 3. Handle pawn promotions
    if (promoted != NO_PIECE) {
        pop_bit(board.bitboards[piece], tgt);
        set_bit(board.bitboards[promoted], tgt);
    }

    // 4. Handle en passant capture
    if (ep) {
        if (board.side == White) pop_bit(board.bitboards[p], tgt - 8);
        else                     pop_bit(board.bitboards[P], tgt + 8);
    }

    // 5. Update en passant target square
    board.en_passant = -1;
    if (dbl) {
        board.en_passant = (board.side == White) ? (src + 8) : (src - 8);
    }

    // 6. Handle castling rook movement
    if (castle) {
        switch (tgt) {
            case 6:  pop_bit(board.bitboards[R], 7);  set_bit(board.bitboards[R], 5);  break; // White O-O
            case 2:  pop_bit(board.bitboards[R], 0);  set_bit(board.bitboards[R], 3);  break; // White O-O-O
            case 62: pop_bit(board.bitboards[r], 63); set_bit(board.bitboards[r], 61); break; // Black O-O
            case 58: pop_bit(board.bitboards[r], 56); set_bit(board.bitboards[r], 59); break; // Black O-O-O
        }
    }

    // 7. Update castling rights
    board.castle &= castling_rights_mask[src];
    board.castle &= castling_rights_mask[tgt];

    // 8. Rebuild occupancy bitboards
    board.occupancies[White] = 0ULL;
    board.occupancies[Black] = 0ULL;
    for (int i = P; i <= K; i++) board.occupancies[White] |= board.bitboards[i];
    for (int i = p; i <= k; i++) board.occupancies[Black] |= board.bitboards[i];
    board.occupancies[Both] = board.occupancies[White] | board.occupancies[Black];

    // 9. Switch side & verify King safety
    int moving_side = board.side;
    board.side = (board.side == White) ? Black : White;

    int king_sq = get_lsb_index((moving_side == White) ? board.bitboards[K] : board.bitboards[k]);
    if (is_square_attacked(king_sq, board.side, board)) {
        board = backup; // Illegal move: restore board and reject
        return false;
    }

    return true;
}