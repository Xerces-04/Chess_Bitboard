#pragma once
#include "board.hpp"
#include "attacks.hpp"
#include "movegen.hpp"
#include <algorithm>

constexpr int INF = 50000;
constexpr int MATE_SCORE = 49000;

// ==========================================
// 1. EVALUATION TABLES (Material + Piece-Square)
// ==========================================
constexpr int material_score[12] = {
    100, 320, 330, 500, 900, 20000,   // White: P, N, B, R, Q, K
   -100,-320,-330,-500,-900,-20000    // Black: p, n, b, r, q, k
};

// Piece-Square Tables written from White's visual perspective (Rank 8 at top -> Rank 1 at bottom).
// Because our board uses LERF (A1=0 at bottom-left, A8=56 at top-left), we mirror the rank index
// with `sq ^ 56` for White and read `sq` directly for Black!
constexpr int pawn_pst[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

constexpr int knight_pst[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};

constexpr int bishop_pst[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};

constexpr int rook_pst[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

constexpr int king_pst[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};

// ==========================================
// 2. STATIC POSITION EVALUATION
// ==========================================
inline int evaluate(const Board& board) {
    int score = 0;

    for (int piece = P; piece <= k; piece++) {
        uint64_t bb = board.bitboards[piece];
        while (bb) {
            int sq = get_lsb_index(bb);
            score += material_score[piece];

            switch (piece) {
                case P: score += pawn_pst[sq ^ 56];   break;
                case N: score += knight_pst[sq ^ 56]; break;
                case B: score += bishop_pst[sq ^ 56]; break;
                case R: score += rook_pst[sq ^ 56];   break;
                case K: score += king_pst[sq ^ 56];   break;
                case p: score -= pawn_pst[sq];        break;
                case n: score -= knight_pst[sq];      break;
                case b: score -= bishop_pst[sq];      break;
                case r: score -= rook_pst[sq];        break;
                case k: score -= king_pst[sq];        break;
                default: break;
            }
            pop_bit(bb, sq);
        }
    }

    // Negamax requires returning score relative to the side to move
    return (board.side == White) ? score : -score;
}

// ==========================================
// 3. MOVE ORDERING (Captures & Promotions First)
// ==========================================
inline int score_move(Move m) {
    int score = 0;
    if (is_move_capture(m)) score += 10000;
    if (get_move_promoted(m) != NO_PIECE) score += 9000;
    return score;
}

inline void sort_moves(MoveList& list) {
    std::sort(list.moves, list.moves + list.count, [](Move a, Move b) {
        return score_move(a) > score_move(b);
    });
}

// ==========================================
// 4. NEGAMAX SEARCH WITH ALPHA-BETA PRUNING
// ==========================================
inline int negamax(Board& board, int depth, int alpha, int beta, int ply) {
    if (depth == 0) {
        return evaluate(board);
    }

    int enemy = (board.side == White) ? Black : White;
    int king_sq = get_lsb_index((board.side == White) ? board.bitboards[K] : board.bitboards[k]);
    bool in_check = is_square_attacked(king_sq, enemy, board);

    // Check extension: never drop straight into evaluation while in check
    if (in_check) depth++;

    MoveList list;
    generate_moves(board, list);
    sort_moves(list);

    int legal_moves = 0;
    int best_score = -INF;

    for (int i = 0; i < list.count; i++) {
        Board copy = board;
        if (!make_move(copy, list.moves[i])) {
            continue; // Skip pseudo-legal moves that leave King in check
        }

        legal_moves++;
        int score = -negamax(copy, depth - 1, -beta, -alpha, ply + 1);

        if (score > best_score) {
            best_score = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            break; // Beta cutoff: opponent won't allow this branch
        }
    }

    // Checkmate or Stalemate detection
    if (legal_moves == 0) {
        if (in_check) {
            return -MATE_SCORE + ply; // Prefer faster checkmates
        }
        return 0; // Stalemate (draw)
    }

    return best_score;
}

// ==========================================
// 5. ROOT SEARCH ENTRY POINT
// ==========================================
inline Move find_best_move(Board& board, int max_depth) {
    Move best_move = 0;
    int best_score = -INF;
    int alpha = -INF;
    int beta = INF;

    MoveList list;
    generate_moves(board, list);
    sort_moves(list);

    for (int i = 0; i < list.count; i++) {
        Board copy = board;
        if (!make_move(copy, list.moves[i])) {
            continue;
        }

        int score = -negamax(copy, max_depth - 1, -beta, -alpha, 1);

        if (score > best_score) {
            best_score = score;
            best_move = list.moves[i];
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return best_move;
}