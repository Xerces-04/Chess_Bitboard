# ♟️ Bitboard Chess Engine (C++ & Raylib)

A fast, lightweight C++ chess game and engine powered by **64-bit Bitboards** and rendered with **Raylib**. Built from the ground up with low-level bitwise board representations, precomputed attack tables, and an integrated AI opponent.

---

## ✨ Features

- **Bitboard-Driven Engine:** Ultra-fast board state tracking, occupancy checks, and move generation using `uint64_t` integers.
- **Interactive Raylib GUI:**
  - Crisp 8×8 board rendering with custom sprite atlas support (`Pieces.png`) and multi-style row switching.
  - Visual move indicators: subtle dots for quiet moves and rings for enemy captures.
  - Highlights for the currently selected square and the previous move's source/target squares.
  - Instant board reset at any time via the `R` key.
- **Built-in Chess AI:** Plays as Black using a configurable search depth (`AI_DEPTH`), evaluating positions after White's turn renders so the UI remains responsive.
- **Full Rule Enforcement:** Pseudo-legal move generation filtered by king-safety checks (`make_move`), auto-Queen pawn promotion, FEN string loading, and automatic Checkmate/Stalemate detection.

---

## 🧠 Bitboard Architecture Highlights

Unlike traditional $8 \times 8$ array-based boards, this engine represents the entire chessboard using **64-bit unsigned integers (`uint64_t`)**, where each bit corresponds to one of the 64 squares.

### 1. LERF Square Mapping (Little-Endian Rank-File)
Squares are indexed from `0` to `63` starting at the bottom-left (`A1 = 0`) up to the top-right (`H8 = 63`):
- **Square Index Formula:** $\text{sq} = \text{rank} \times 8 + \text{file}$
- **Screen Coordinate Translation:** The GUI flips the vertical axis (`7 - rank`) so that Rank 8 renders cleanly at the top of the window while keeping internal engine math strictly Little-Endian.

### 2. Piece & Occupancy Bitboards
The `Board` structure maintains separate bitboards for rapid bitwise queries:
- **12 Piece Bitboards (`board.bitboards[P..k]`):** One `uint64_t` per piece type and color (`P, N, B, R, Q, K` for White; `p, n, b, r, q, k` for Black).
- **3 Occupancy Bitboards (`board.occupancies[White/Black/Both]`):** Composite bitmasks updated on every move to check square occupation in $O(1)$ CPU cycles via `get_bit()`.

### 3. Bitwise Iteration & Manipulation
When rendering pieces or evaluating attacks, the engine avoids looping over all 64 squares. Instead, it isolates active pieces directly using Least Significant Bit (LSB) scanning:
```cpp
for (int piece = P; piece <= k; piece++) {
    uint64_t bb = board.bitboards[piece];
    while (bb) {
        int sq = get_lsb_index(bb); // Extract index of lowest set bit
        draw_piece(piece, square_to_screen(sq), myAtlas, 0);
        pop_bit(bb, sq);            // Clear the bit (bb &= bb - 1)
    }
}
```
This ensures the loop only runs for the exact number of pieces currently on the board.

### 4. Packed Integer Move Encoding
Moves are encoded into compact integer primitives (`Move`) rather than heavy objects. Bitwise masks and shifts extract move metadata with zero heap allocations:
- `get_move_source(m)` — Origin square (`0–63`)
- `get_move_target(m)` — Destination square (`0–63`)
- `get_move_promoted(m)` — Promotion piece flag

### 5. Precomputed Leaper Attack Tables
On startup, `init_leaper_attacks()` populates lookup tables for non-sliding pieces (Pawns, Knights, and Kings), allowing attack and check detection (`is_square_attacked`) to execute via single bitwise `AND` operations.

---

## 🛠️ Project Structure

```text
├── main.cpp            # Raylib window loop, input handling, rendering, and AI trigger
├── board.hpp           # Bitboard state, FEN parser, bit macros, and move making
├── attacks.hpp         # Precomputed leaper tables and square attack detection
├── movegen.hpp         # Pseudo-legal move generation & packed Move encoding
├── search.hpp          # AI search algorithm (find_best_move)
└── assets/
    └── images/art/collection/Sprite_Atlases/
        └── Pieces.png  # 12-column sprite sheet mapped to internal Piece enums
```

---

## 🚀 Building & Running

### Prerequisites
- A C++17 (or newer) compiler (`g++`, `clang++`, or MSVC)
- [Raylib](https://www.raylib.com/) installed and linked

### Compile (Linux / macOS)
```bash
g++ -O3 main.cpp -o bitboard_chess -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./bitboard_chess
```

### Compile (Windows - MinGW)
```bash
g++ -O3 main.cpp -o bitboard_chess.exe -lraylib -lopengl32 -lgdi32 -lwinmm
bitboard_chess.exe
```

---

## 🎮 Controls & Configuration

| Input / Constant | Action / Description |
| :--- | :--- |
| **Left Click** | Select a White piece or click a highlighted square to move |
| **`R` Key** | Reset the board to the standard starting FEN position |
| `AI_DEPTH` | Defined in `main.cpp` (default `4`). Increase (`5+`) for stronger play or decrease (`3`) for instant responses |
| `SQUARE_SIZE` | Defined in `main.cpp` (default `90px`, creating a `720×720` window) |