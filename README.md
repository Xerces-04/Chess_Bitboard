# ⛳ Mini Golf 2D

<p align="center">
  <img src="assets//images/Golf64_main.png" alt="Mini Golf 2D Gameplay Preview" width="500" />
</p>

A classic 2D Mini Golf game built from scratch in C++ using the **[Raylib](https://www.raylib.com/)** library. Aim, adjust your power, dodge obstacles, and sink the ball across 9 challenging holes with the lowest score possible.

---

## 🎮 Features

* **9 Custom Hole Layouts:** Progress through distinct levels featuring walls, narrow corridors, and angled obstacles.
* **Drag-and-Release Aiming:** Intuitive slingshot-style mouse controls with a dynamic direction and power indicator.
* **Persistent Course Records:** Automatically saves and loads your lowest stroke record to `courseRecord.txt`.
* **Pixel Font UI:** Custom HUD displaying current hole, stroke counter, and end-of-game score summaries.

---

## 🕹️ Controls

| Input | Action |
| :--- | :--- |
| **Left Mouse Button (Click & Drag)** | Aim ball direction and charge stroke power |
| **Left Mouse Button (Release)** | Shoot the ball |
| **Enter** | Restart game from Hole 1 (Game Over screen) |
| **Escape** | Exit game |

---

## 💻 Technical Highlights & Skills

* **Vector Mathematics & 2D Physics:** Calculated normalized directional vectors using Euclidean distance formulas to translate 2D mouse drag input into proportional launch velocities.
* **Object-Oriented Design (OOP):** Modular architecture separating core concerns into distinct classes (`Game`, `Level`, `Ball`, `Hole`, `Arrow`) for maintainable state management and object interaction.
* **Custom Level & Collision Architecture:** Designed a data-driven level setup utilizing `std::vector<Rectangle>` collections to define complex obstacles, collision bounds, and custom spawn points across 9 distinct stages.
* **Persistent File I/O:** Implemented non-volatile high-score tracking using C++ standard file streams (`std::ifstream` / `std::ofstream`) to serialize and load player course records.
* **Game Loop & State Control:** Managed a strict 60 FPS update-render loop, dynamic HUD rendering with custom TrueType fonts (`LoadFontEx`), and clean state transitions between active gameplay, hole progression, and game-over screens.

---

## 📁 Project Structure

```text
├── assets/
│   ├── font/
│   │   └── monogram.ttf      # Custom UI typography
│   └── courseRecord.txt      # Saved high score data
├── src/
│   ├── main.cpp              # Game loop and UI rendering
│   ├── game.cpp              # Game state, level loading, input handling
│   ├── game.hpp              # Game class declarations
│   ├── level.hpp             # Level definitions and obstacle lists
│   ├── ball.hpp              # Ball physics and rendering
│   ├── hole.hpp              # Hole target coordinates and radius
│   └── arrow.hpp             # Aiming indicator logic
├── courseRecord.txt          # Root record save file fallback
└── README.md

