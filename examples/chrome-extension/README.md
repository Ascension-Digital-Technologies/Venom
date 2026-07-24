# Velocity Chess Browser Engine

A local-first chess engine that understands browser-rendered chessboards visually and interacts with them through natural, humanized pointer movement.

Velocity Chess Browser Engine reconstructs a chess position from the page's rendered squares and pieces, detects the player's side, searches for a move locally, and plays through the same visible board controls a person would use. It does not require FEN, a chess API, a remote server, or access to the website's internal game engine.

> Use only on websites you own or are explicitly authorized to automate.

## Highlights

- **Visual board reconstruction** — reads common DOM-rendered chessboards directly from squares, piece elements, labels, classes, images, and coordinates.
- **Local chess engine** — move generation, evaluation, search, hashing, and transposition tables run entirely inside the extension.
- **Automatic side detection** — identifies whether the local player is White or Black from page signals, interactivity, and board orientation.
- **Stateful turn tracking** — combines visible turn hints, opening-state inference, and exact board deltas.
- **Humanized interaction** — curved movement, hover pauses, drag arcs, micro-corrections, destination hesitation, and varied release timing.
- **Skill simulation** — seven levels from Beginner to Maximum choose among ranked legal candidates using contextual accuracy rather than flat random blunders.
- **Playing styles** — Balanced, Aggressive, Tactical, Positional, and Defensive profiles influence candidate preferences.
- **Stable player personality** — each game receives consistent speed, hesitation, accuracy, aggression, and cursor-directness traits.
- **Variable move cadence** — optionally gives every move a distinct thinking and interaction profile.
- **Match presets** — tuned modes for 1-minute, 3-minute, 5-minute, and 10-minute games.
- **Strict allowlist** — activation is restricted by `allowed_sites.txt` and temporary `activeTab` access.
- **No remote runtime** — no API, cloud engine, telemetry, background worker, or extension message bus.

## How it works

```text
Rendered chessboard
       │
       ▼
Visual board reader
  ├─ finds squares and pieces
  ├─ resolves orientation
  ├─ detects the player's side
  └─ tracks board changes
       │
       ▼
Position reconstruction
  ├─ piece placement
  ├─ side to move
  ├─ castling history
  └─ en-passant history
       │
       ▼
Velocity Chess engine
  ├─ legal move generation
  ├─ evaluation
  ├─ alpha-beta search
  ├─ quiescence search
  └─ transposition table
       │
       ▼
Humanized browser interaction
  ├─ approach and hover
  ├─ mouse down / selection
  ├─ curved drag path
  └─ release / click fallback
```

## Supported boards

The visual reader supports common DOM-based board implementations, including:

- algebraic classes such as `.square-e4`;
- numeric square classes such as `.square-52`;
- `data-square`, `data-cell`, and `data-coord` attributes;
- absolutely positioned piece elements;
- piece identity from classes, image filenames, accessible labels, text, and Unicode symbols;
- White- and Black-oriented boards;
- drag-and-drop and click-to-move interfaces.

A chessboard rendered as one opaque `<canvas>` cannot be reconstructed from the DOM alone. Canvas-based sites need accessible overlays or a small page adapter.

## Skill and humanization

Velocity keeps engine analysis separate from human decision-making. The engine ranks several legal candidates, then the selected skill level and style choose among them. Lower levels make context-sensitive inaccuracies more often in complex positions; forced moves remain highly reliable. Candidate selection is constrained by skill-scaled loss bands, preventing implausible catastrophic choices unless the selected level and position genuinely justify them. Maximum mode always uses the strongest engine choice.

| Skill | Approximate level | Candidate behavior |
|---|---:|---|
| Beginner | ~700 | Broad choices, shallow tactical vision, frequent inaccuracies |
| Casual | ~1000 | Basic tactics with noticeable inconsistency |
| Intermediate | ~1300 | Reasonable tactical awareness and moderate accuracy |
| Club | ~1600 | Strong practical play with occasional human mistakes |
| Advanced | ~1900 | High accuracy and deeper calculation |
| Expert | ~2200 | Rare inaccuracies and narrow candidate selection |
| Maximum | Full strength | Strongest engine move, no skill noise |

Humanization also controls how a selected move is performed.

- curved approach paths;
- variable source-square hover time;
- mouse-down and selection holds;
- subtle aiming corrections;
- natural drag curvature;
- occasional controlled overshoot and correction;
- destination hesitation;
- varied release timing;
- visible simulated cursor overlay.

The **Different time every move** option independently varies thinking, hovering, holding, dragging, and releasing while avoiding near-identical consecutive timing profiles.

The extension cannot move the operating system's physical cursor. It renders a local cursor overlay and dispatches pointer, mouse, and HTML5 drag/drop events to the board.

## Match modes

| Mode | Intended pace | Default engine budget | Interaction style |
|---|---|---:|---|
| 1m | Bullet | 180 ms | Compact decisions and fast drags |
| 3m | Blitz | 450 ms | Short, varied pauses |
| 5m | Blitz | 800 ms | Balanced strength and pacing |
| 10m | Rapid | 1300 ms | Deeper search and calmer movement |

Advanced controls can override search depth, search time, thinking delay, and movement duration.

## Installation

1. Clone or download this repository.
2. Add your authorized website to `allowed_sites.txt`.
3. Open `chrome://extensions`.
4. Enable **Developer mode**.
5. Select **Load unpacked**.
6. Choose the repository root.
7. Refresh the chess page, open the extension, and press **Start**.

Example allowlist:

```text
https://chess.example.com/*
http://localhost:8080/*
```

After editing `allowed_sites.txt`, reload the extension from `chrome://extensions`.

## Optional page adapter

Most DOM-rendered boards work without integration. A website can optionally provide stronger visual hints or its own move submission function:

```js
window.VelocityChessAdapter = {
  getVisualHints() {
    return {
      turn: currentPlayer === "black" ? "b" : "w",
      playerSide: localPlayerColor === "black" ? "b" : "w",
      gameOver: Boolean(gameFinished)
    };
  },

  async playMove({ from, to, promotion }) {
    await submitMove({ from, to, promotion });
  }
};
```

## Project structure

```text
engine/                 Velocity Chess engine modules
lib/                    Shared extension utilities
tests/                  Node-based unit and regression tests
.github/                CI and GitHub community files
content-script.js       Page controller and engine orchestration
visual-board.js         DOM board discovery and position tracking
humanize.js             Pointer path and timing generation
skill-levels.js          Skill, style, personality, and move-selection model
match-modes.js          Bullet, blitz, rapid, and custom presets
page-bridge.js          Optional page-level adapter bridge
popup.*                 Extension controls
allowed_sites.txt       Explicit activation allowlist
manifest.json           Chrome Manifest V3 definition
```

## Development

Requirements:

- Node.js 20 or newer
- Google Chrome or Chromium 102 or newer

Install and test:

```bash
npm install
npm test
npm run check
```

Individual commands:

```bash
node --test tests/*.test.mjs
node --check content-script.js
node --check visual-board.js
node --check humanize.js
node --check match-modes.js
node --check skill-levels.js
node --check page-bridge.js
node --check popup.js
```

## Security and privacy

- The engine and visual reader run locally.
- No gameplay data is uploaded.
- No telemetry is collected.
- No background service worker is used.
- No extension message bus or long-lived port is used.
- No broad host permission is requested.
- The active page is accessed only after an explicit user action.
- `allowed_sites.txt` is checked before controller injection.
- Global wildcard patterns are intentionally rejected.

See [SECURITY.md](SECURITY.md) for reporting guidance and the supported security model.

## Limitations

- Start near the beginning of a game for reliable castling and en-passant history.
- A single visual snapshot cannot reveal whether a king or rook moved earlier and returned.
- Heavily animated boards may require a brief stabilization period.
- Canvas-only boards require accessible overlays or an adapter.
- Site markup changes may require a new board detector profile.

## Contributing

Bug reports, board compatibility improvements, tests, and performance work are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

## License

Velocity Chess Browser Engine is released under the [MIT License](LICENSE).

## Resilient restart behavior

Every Start performs a clean live-board reconstruction. If animations or tab suspension skip visual transitions, the tracker reseeds from the current rendered position instead of depending on stale history.


### Learned recorded paths

The movement planner blends the closest recorded strokes and calculates a fresh distance-aware trajectory for every move. It learns curvature, acceleration, corrections, and duration while retaining a procedural fallback.

## Diagnostics and profile quality

Version 0.11.0 adds a popup health dashboard with board-read age, recovery counts, the latest move, mouse-profile quality, distance-coverage guidance, and a privacy-safe copy-diagnostics action. The preview canvas overlays recent normalized recordings and the blended learned trajectory. Low-confidence distance matches automatically use the procedural path generator.

