#include "raylib.h"
#include "board.hpp"
#include "attacks.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include <vector>
#include <string>

constexpr int SQUARE_SIZE = 90;
constexpr int BOARD_PIXELS = SQUARE_SIZE * 8; // 720x720
constexpr int AI_DEPTH = 4;                   // Adjust (3-5) for speed vs. strength

// Convert LERF square (0=A1 .. 63=H8) to screen coordinates (top-left of square)
Vector2 square_to_screen(int sq) {
    int rank = sq / 8;
    int file = sq % 8;
    float x = static_cast<float>(file * SQUARE_SIZE);
    float y = static_cast<float>((7 - rank) * SQUARE_SIZE); // Flip Y so Rank 8 is at top
    return { x, y };
}

// Convert mouse pixel position to LERF square (0..63), or -1 if out of bounds
int screen_to_square(Vector2 pos) {
    int file = static_cast<int>(pos.x) / SQUARE_SIZE;
    int visual_rank = static_cast<int>(pos.y) / SQUARE_SIZE;
    if (file < 0 || file > 7 || visual_rank < 0 || visual_rank > 7) return -1;
    int rank = 7 - visual_rank;
    return rank * 8 + file;
}

// Helper to get all strictly legal moves for the current board position
std::vector<Move> get_legal_moves(Board& board) {
    std::vector<Move> legal;
    MoveList pseudo;
    generate_moves(board, pseudo);

    for (int i = 0; i < pseudo.count; i++) {
        Board copy = board;
        if (make_move(copy, pseudo.moves[i])) {
            legal.push_back(pseudo.moves[i]);
        }
    }
    return legal;
}

// Draw a crisp vector badge + letter for each piece (zero external assets needed)
// void draw_piece(int piece, Vector2 top_left) {
//     const char* labels[] = { "P", "N", "B", "R", "Q", "K", "P", "N", "B", "R", "Q", "K" };
//     bool is_white = (piece <= K);

//     Vector2 center = { top_left.x + SQUARE_SIZE / 2.0f, top_left.y + SQUARE_SIZE / 2.0f };
//     Color fill_col   = is_white ? RAYWHITE : Color{ 35, 35, 40, 255 };
//     Color border_col = is_white ? Color{ 40, 40, 45, 255 } : RAYWHITE;
//     Color text_col   = is_white ? Color{ 25, 25, 30, 255 } : RAYWHITE;

//     DrawCircleV(center, SQUARE_SIZE * 0.36f, fill_col);
//     DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), SQUARE_SIZE * 0.36f, border_col);

//     int font_size = 38;
//     int text_w = MeasureText(labels[piece], font_size);
//     DrawText(labels[piece],
//              static_cast<int>(center.x - text_w / 2.0f),
//              static_cast<int>(center.y - font_size / 2.0f),
//              font_size,
//              text_col);
// }

void draw_piece(int piece, Vector2 top_left, const PieceAtlas& atlas, int style_row = 0) {
    // Index = your internal Piece enum (P, N, B, R, Q, K, p, n, b, r, q, k)
    // Value = column index (0-11) in Pieces.png
    static constexpr int ATLAS_COL[12] = {
        5,  // P -> column 5 (White Pawn)
        4,  // N -> column 4 (White Knight)
        3,  // B -> column 3 (White Bishop)
        2,  // R -> column 2 (White Rook)
        1,  // Q -> column 1 (White Queen)
        0,  // K -> column 0 (White King)
        11, // p -> column 11 (Black Pawn)
        10, // n -> column 10 (Black Knight)
        9,  // b -> column 9 (Black Bishop)
        8,  // r -> column 8 (Black Rook)
        7,  // q -> column 7 (Black Queen)
        6   // k -> column 6 (Black King)
    };

    int col = ATLAS_COL[piece];

    Rectangle source = {
        col * atlas.pieceWidth,
        style_row * atlas.pieceHeight,
        atlas.pieceWidth,
        atlas.pieceHeight
    };

    float padding = SQUARE_SIZE * 0.1f;
    Rectangle dest = {
        top_left.x + padding,
        top_left.y + padding,
        SQUARE_SIZE - (padding * 2.0f),
        SQUARE_SIZE - (padding * 2.0f)
    };

    DrawTexturePro(atlas.sheet, source, dest, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);
}

int main() {
    // 1. Initialize engine tables & board
    init_leaper_attacks();

    Board board;
    board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::vector<Move> legal_moves = get_legal_moves(board);
    int selected_sq = -1;
    Move last_move = 0;
    std::string status_text = "";


    // 2. Initialize Raylib window
    InitWindow(BOARD_PIXELS, BOARD_PIXELS, "Bitboard Chess - Raylib");
    SetTargetFPS(60);

    Color light_sq = { 238, 238, 210, 255 };
    Color dark_sq  = { 118, 150, 86, 255 };


    
    PieceAtlas myAtlas;
    myAtlas.sheet = LoadTexture("assets/images/art/collection/Sprite_Atlases/Pieces.png"); 
    myAtlas.pieceWidth = (float)myAtlas.sheet.width / 12; //12 columns in the sprite sheet
    myAtlas.pieceHeight = (float)myAtlas.sheet.height / 100; //100n rows in the sprite sheet


    while (!WindowShouldClose()) {
        // ==========================================
        // 3. INPUT HANDLING (Human plays White)
        // ==========================================
        if (board.side == White && status_text.empty() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            int clicked_sq = screen_to_square(GetMousePosition());

            if (clicked_sq != -1) {
                if (selected_sq == -1) {
                    // Select a square if it holds a White piece
                    if (get_bit(board.occupancies[White], clicked_sq)) {
                        selected_sq = clicked_sq;
                    }
                } else {
                    // Try to find a legal move from selected_sq -> clicked_sq
                    Move chosen_move = 0;
                    for (Move m : legal_moves) {
                        if (get_move_source(m) == selected_sq && get_move_target(m) == clicked_sq) {
                            // Auto-promote to Queen if pawn reaches Rank 8
                            int promo = get_move_promoted(m);
                            if (promo == NO_PIECE || promo == Q) {
                                chosen_move = m;
                                break;
                            }
                        }
                    }

                    if (chosen_move != 0) {
                        make_move(board, chosen_move);
                        last_move = chosen_move;
                        selected_sq = -1;
                        legal_moves = get_legal_moves(board);

                        if (legal_moves.empty()) {
                            int b_king = get_lsb_index(board.bitboards[k]);
                            status_text = is_square_attacked(b_king, White, board)
                                ? "Checkmate! White Wins!"
                                : "Stalemate! Draw.";
                        }
                    } else if (get_bit(board.occupancies[White], clicked_sq)) {
                        // Switch selection to another friendly piece
                        selected_sq = clicked_sq;
                    } else {
                        selected_sq = -1;
                    }
                }
            }
        }

        // Press R at any time to reset the game
        if (IsKeyPressed(KEY_R)) {
            board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            legal_moves = get_legal_moves(board);
            selected_sq = -1;
            last_move = 0;
            status_text = "";
        }

        // ==========================================
        // 4. RENDERING
        // ==========================================
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw 8x8 grid
        for (int sq = 0; sq < 64; sq++) {
            int rank = sq / 8;
            int file = sq % 8;
            bool is_light = ((rank + file) % 2 != 0);
            Vector2 pos = square_to_screen(sq);

            DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                          SQUARE_SIZE, SQUARE_SIZE, is_light ? light_sq : dark_sq);

            // Highlight last move squares
            if (last_move != 0 && (sq == get_move_source(last_move) || sq == get_move_target(last_move))) {
                DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                              SQUARE_SIZE, SQUARE_SIZE, Color{ 246, 246, 105, 130 });
            }

            // Highlight currently selected square
            if (sq == selected_sq) {
                DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                              SQUARE_SIZE, SQUARE_SIZE, Color{ 255, 215, 0, 160 });
            }
        }

        // Draw legal destination hints for the selected piece
        if (selected_sq != -1) {
            for (Move m : legal_moves) {
                if (get_move_source(m) == selected_sq) {
                    int tgt = get_move_target(m);
                    Vector2 pos = square_to_screen(tgt);
                    Vector2 center = { pos.x + SQUARE_SIZE / 2.0f, pos.y + SQUARE_SIZE / 2.0f };

                    if (get_bit(board.occupancies[Both], tgt)) {
                        DrawRing(center, SQUARE_SIZE * 0.38f, SQUARE_SIZE * 0.46f, 0, 360, 32, Color{ 0, 0, 0, 80 });
                    } else {
                        DrawCircleV(center, SQUARE_SIZE * 0.14f, Color{ 0, 0, 0, 80 });
                    }
                }
            }
        }

        // Draw pieces from bitboards
        for (int piece = P; piece <= k; piece++) {
            uint64_t bb = board.bitboards[piece];
            while (bb) {
                int sq = get_lsb_index(bb);
                draw_piece(piece, square_to_screen(sq), myAtlas, 0); // Use style_row=0 for default piece set
                pop_bit(bb, sq);
            }
        }

        // Draw Game Over banner if applicable
        if (!status_text.empty()) {
            DrawRectangle(0, BOARD_PIXELS / 2 - 50, BOARD_PIXELS, 100, Color{ 0, 0, 0, 200 });
            int tw = MeasureText(status_text.c_str(), 36);
            DrawText(status_text.c_str(), (BOARD_PIXELS - tw) / 2, BOARD_PIXELS / 2 - 28, 36, RAYWHITE);
            const char* sub = "Press [R] to Play Again";
            int sw = MeasureText(sub, 20);
            DrawText(sub, (BOARD_PIXELS - sw) / 2, BOARD_PIXELS / 2 + 16, 20, LIGHTGRAY);
        }

        EndDrawing();

        // ==========================================
        // 5. AI TURN (Runs after White's move renders)
        // ==========================================
        if (board.side == Black && status_text.empty()) {
            Move ai_move = find_best_move(board, AI_DEPTH);
            if (ai_move != 0) {
                make_move(board, ai_move);
                last_move = ai_move;
            }
            legal_moves = get_legal_moves(board);

            if (legal_moves.empty()) {
                int w_king = get_lsb_index(board.bitboards[K]);
                status_text = is_square_attacked(w_king, Black, board)
                    ? "Checkmate! Black Wins!"
                    : "Stalemate! Draw.";
            }
        }
    }

    CloseWindow();
    return 0;
}