# Architecture

## Runtime boundaries

The extension has four primary layers:

1. **Popup controls** configure the session and inject the local runtime.
2. **Visual board reader** discovers the board, maps pieces to squares, resolves orientation, and tracks changes.
3. **Velocity Chess engine** reconstructs the position and searches locally.
4. **Humanized interaction layer** converts a selected move into pointer, mouse, drag/drop, or click events.

No network service is required.

## Visual state model

The board reader builds stable snapshots rather than trusting a single animation frame. Consecutive snapshots are compared to infer moves and preserve state that is not visible from piece placement alone.

Tracked state includes:

- local player side;
- board orientation;
- side to move;
- castling eligibility inferred from observed movement;
- en-passant opportunity inferred from the last move;
- previous stable board snapshot;
- game reset detection.

## Engine pipeline

The bundled engine uses:

- compact move encoding;
- legal move generation;
- static evaluation;
- iterative search;
- alpha-beta pruning;
- quiescence search;
- Zobrist hashing;
- transposition-table caching;
- move ordering heuristics.

## Interaction pipeline

A move is attempted through progressively compatible mechanisms:

1. optional page adapter;
2. pointer/mouse drag interaction;
3. HTML5 drag/drop events;
4. click-to-move fallback.

Humanization controls path geometry and event timing without changing the selected chess move.
