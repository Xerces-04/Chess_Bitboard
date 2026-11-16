#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <cctype>
#include <iostream>

// Enums & Bit Helpers stay outside the struct so every file can use them
enum Piece { P, N, B, R, Q, K, p, n, b, r, q, k, NO_PIECE };
enum Piece_Color { White, Black, Both };
enum Castling { WK = 1, WQ = 2, BK = 4, BQ = 8 };

inline void set_bit(uint64_t &bb, int square) { bb |= (1ULL << square); }
inline void pop_bit(uint64_t &bb, int square) { bb &= ~(1ULL << square); }
inline bool get_bit(uint64_t bb, int square)  { return (bb & (1ULL << square)) != 0; }

struct Board {
    // 1. Member Variables
    uint64_t bitboards[12] = {0ULL};
    uint64_t occupancies[3] = {0ULL};
    int side = White;
    int en_passant = -1;
    int castle = 0;
    int halfmove_clock = 0;
    int fullmove_number = 1;

    // 2. Member Functions (have direct access to the variables above)
    void load_fen(const std::string& fen) {
        for (int i = 0; i < 12; i++) bitboards[i] = 0ULL;
        for (int i = 0; i < 3; i++)  occupancies[i] = 0ULL;
        side = White;
        en_passant = -1;
        castle = 0;

        std::istringstream iss(fen);
        std::string board_part, turn_part, castle_part, ep_part;
        iss >> board_part >> turn_part >> castle_part >> ep_part >> halfmove_clock >> fullmove_number;

        int rank = 7;
        int file = 0;

        for (char c : board_part) {
            if (c == '/') {
                rank--;
                file = 0;
            } else if (std::isdigit(c)) {
                file += (c - '0');
            } else {
                int square = rank * 8 + file;
                int piece = NO_PIECE;

                switch (c) {
                    case 'P': piece = P; break;
                    case 'N': piece = N; break;
                    case 'B': piece = B; break;
                    case 'R': piece = R; break;
                    case 'Q': piece = Q; break;
                    case 'K': piece = K; break;
                    case 'p': piece = p; break;
                    case 'n': piece = n; break;
                    case 'b': piece = b; break;
                    case 'r': piece = r; break;
                    case 'q': piece = q; break;
                    case 'k': piece = k; break;
                }

                if (piece != NO_PIECE) {
                    set_bit(bitboards[piece], square);
                }
                file++;
            }
        }

        side = (turn_part == "w") ? White : Black;

        for (char c : castle_part) {
            switch (c) {
                case 'K': castle |= WK; break;
                case 'Q': castle |= WQ; break;
                case 'k': castle |= BK; break;
                case 'q': castle |= BQ; break;
            }
        }

        if (ep_part != "-") {
            int ep_file = ep_part[0] - 'a';
            int ep_rank = ep_part[1] - '1';
            en_passant = ep_rank * 8 + ep_file;
        }

        for (int i = P; i <= K; i++) occupancies[White] |= bitboards[i];
        for (int i = p; i <= k; i++) occupancies[Black] |= bitboards[i];
        occupancies[Both] = occupancies[White] | occupancies[Black];
    }

    void print_board() const {
        const char piece_chars[] = "PNBRQKpnbrqk";
        std::cout << "\n";
        for (int rank = 7; rank >= 0; rank--) {
            std::cout << "  " << (rank + 1) << "   ";
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                int piece = NO_PIECE;

                for (int bb_idx = P; bb_idx <= k; bb_idx++) {
                    if (get_bit(bitboards[bb_idx], square)) {
                        piece = bb_idx;
                        break;
                    }
                }
                std::cout << (piece == NO_PIECE ? '.' : piece_chars[piece]) << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n      a b c d e f g h\n\n";
        std::cout << "      Side:     " << (side == White ? "White" : "Black") << "\n";
        std::cout << "      EnPass:   " << en_passant << "\n";
        std::cout << "      Castling: "
                  << ((castle & WK) ? 'K' : '-')
                  << ((castle & WQ) ? 'Q' : '-')
                  << ((castle & BK) ? 'k' : '-')
                  << ((castle & BQ) ? 'q' : '-') << "\n\n";
    }

    void print_bitboard(uint64_t bb) {
        std::cout << "\n";
        
        // Loop ranks from top (Rank 8 = index 7) down to bottom (Rank 1 = index 0)
        for (int rank = 7; rank >= 0; rank--) {
            // Print rank number on the left edge
            std::cout << "  " << (rank + 1) << "   ";

            // Loop files from left (File A = index 0) to right (File H = index 7)
            for (int file = 0; file < 8; file++) {
                // Convert (rank, file) into a 0..63 bit index
                int square = rank * 8 + file;

                // Check if the bit at 'square' is set
                bool is_set = (bb & (1ULL << square)) != 0;

                std::cout << (is_set ? "1 " : ". ");
            }
            std::cout << "\n";
        }

        // Print file labels along the bottom edge
        std::cout << "\n      a b c d e f g h\n\n";
        
        // Print the raw 64-bit decimal value (useful for debugging constants)
        std::cout << "      Bitboard: " << bb << "ULL\n\n";
    }

};

struct PieceAtlas {
    Texture2D sheet;
    float pieceWidth;
    float pieceHeight;
};

// Use "inline" to prevent redefinition errors if included in multiple files
// inline Rectangle GetPieceRect(PieceAtlas atlas, int suit, int value) {
//     return (Rectangle){
//         .x = (float)(value - 1) * atlas.cardWidth,
//         .y = (float)suit * atlas.cardHeight,
//         .width = atlas.cardWidth,
//         .height = atlas.cardHeight
//     };
// }