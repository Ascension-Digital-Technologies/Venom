## 0.8.0 - 2026-07-19

- Added active-board scoring so duplicate or stale DOM boards no longer win simply because they scan first.
- Added board-root affinity and automatic switching when the rendered board is replaced.
- Added frozen-position health probes and proactive visual-state rebuilding while it is the engine's turn.
- Added board confidence, root connectivity, snapshot age, and proactive recovery telemetry.
- Expanded recoverable visual error coverage for incomplete and temporarily hidden boards.

# Changelog

## 0.11.0

- Added live engine and board-health diagnostics in the popup.
- Added privacy-safe copyable recovery and movement diagnostics.
- Added recorded mouse-profile quality scoring and coverage guidance.
- Added distance-aware confidence gating with automatic procedural fallback.
- Added a replay visualizer showing recent recordings and the blended learned path.
- Upgraded the controller/page bridge namespace to V0110.

## 0.10.0

- Calculates a new mouse trajectory from the complete recorded profile for every movement.
- Blends the closest recorded strokes by distance instead of replaying one template verbatim.
- Learns movement duration scaling from recorded stroke distance and timing.
- Preserves recorded acceleration, curvature, correction, and pause timing through normalized interpolation.
- Adds bounded natural variation so repeated moves do not produce identical paths.
- Falls back to procedural motion whenever the profile is missing or invalid.

## 0.9.1

- Made recorded mouse profiles completely fail-open during controller startup.
- Added a 350 ms profile-load ceiling so extension storage can never stall board initialization.
- Validated and sanitized saved movement templates before they reach the motion generator.
- Stopped installing global pointer listeners until recording is explicitly started.
- Removed pointer listeners immediately after saving or clearing a recording.
- Tolerated unavailable or invalidated extension storage without stopping normal play.

## 0.9.0

- Added opt-in real mouse-stroke recording on authorized local or owned pages.
- Stores privacy-preserving normalized trajectory templates rather than raw screen coordinates or clicked content.
- Replays learned curvature, acceleration, pauses, corrections, and timing through the existing visual interaction layer.
- Added popup controls to record, save, and clear a movement profile.
- Added Chrome local-storage support for the saved movement profile.


## 0.7.0 - Adaptive human rhythm

- Added fatigue recovery during forced and low-complexity sequences instead of allowing fatigue to rise forever.
- Added natural tilt and confidence decay between decisions.
- Added short self-correction periods after mistakes, reducing unrealistic clusters of consecutive errors.
- Added complex-position streak tracking so sustained difficult play creates more strain than isolated hard moves.
- Added time-debt-aware pacing: unusually long thinks are gradually repaid on later non-critical moves.
- Expanded human-state observations with selected rank, mistake type, forced-move state, thinking time, and clock pressure.


## 0.6.0 - Bounded human error model

- Added skill-scaled centipawn-loss bands so inaccuracies remain believable instead of selecting catastrophic outliers.
- Added time-pressure influence to move uncertainty and mistake probability when clock data is available.
- Added score-loss, loss-limit, and time-pressure telemetry for every played move.
- Replaced obsolete SESSION_START page-bridge tests with tests for the current RESET_STATE and fail-closed command contract.
- Restored a fully passing test suite.

## 0.5.0 - Human decision model

- Reworked position complexity scoring around candidate closeness, ambiguity, tactical volatility, forcing moves, legal-move density, check state, and game phase.
- Added clock-aware thought budgeting with a reserve, estimated moves remaining, increment support, and a hard per-move cap.
- Added faster responses for forced moves, obvious winning positions, known positions, and familiar opening play.
- Added longer pauses for surprising evaluation changes, close candidate choices, and volatile tactical positions.
- Preserved deterministic personality behavior, skill-specific candidate selection, recovery, and full-strength maximum mode.


All notable project changes are documented here.

## [0.4.5] - 2026-07-18

### Fixed

- Detects the two visually highlighted last-move squares on flipped and normal boards.
- Supports Chessboard.js-style `highlight1`/`highlight2`, last-move attributes, and move-source/destination classes.
- Adds a background-color fallback for boards that paint last-move squares without semantic classes.
- Infers the previous mover from the piece currently occupying the highlighted destination square.
- Passes last-move evidence into restart and resync turn reconstruction, preventing Black-side deadlocks in active games.
- Added a regression test for the flipped-board position where White moved from f6 to g7 and Black must respond.

## [0.4.4] - 2026-07-18

### Fixed

- Hardened visual board discovery against captured-piece trays, promotion overlays, duplicate animation layers, and decorative chess icons.
- Rejects tiny SVG descendants and oversized transition elements before piece classification.
- Scores unique occupied squares rather than raw piece-like DOM node counts.
- Tries multiple plausible board roots and selects the first coherent position containing exactly one king per side.
- Improved incomplete-scan diagnostics while retaining stable-frame retry behavior.

## [0.4.3] - 2026-07-18

### Added

- Automatic live-board tracker reseeding when complex animations or tab suspension skip multiple visual transitions.
- Six-attempt restart bootstrap that waits for a stable rendered board before giving up.
- Consecutive-failure recovery accounting with lifetime recovery telemetry.
- Low-confidence orientation fallback so a stable board never fails solely because a turn label is absent.

### Changed

- Stable-frame detection now polls for up to 1.4 seconds instead of relying on two short fixed animation delays.
- The recovery budget resets after every successful board read or completed move, allowing future recoveries throughout long games.
- Mid-game Stop → Start always rebuilds from the current rendered placement and conservatively clears unprovable en-passant and castling history.

## [0.4.2] - 2026-07-18

### Added

- Per-game fatigue, confidence, and tilt state.
- Familiar-position memory for faster repeated-position responses.
- Surprise-aware thinking time based on evaluation swings.
- Skill-specific mistake archetypes instead of generic random inaccuracies.
- Rare complexity-sensitive hesitation behavior.
- Local in-memory telemetry for move rank, score loss, timing, complexity, and fatigue.

## [0.4.1] - 2026-07-18

### Added

- Hard visual-state reconstruction on every Stop → Start cycle.
- Bounded self-recovery for transient board animation, tracker, move-acceptance, and visual timeout failures.
- Recovery status and counters exposed through the controller status API.

### Fixed

- Restarting mid-game no longer reuses stale page-side board history or snapshots.
- The page cursor and visual tracker are fully discarded before a restarted session reads the live position.

## 0.4.0

- Added seven selectable skill levels from Beginner through Maximum.
- Added ranked multi-candidate engine analysis for human-like move selection.
- Added Balanced, Aggressive, Tactical, Positional, and Defensive playing styles.
- Added Natural, Fast, Careful, and Highly Varied behavior profiles.
- Added stable per-game personality seeds for consistent timing, accuracy, and cursor behavior.
- Added contextual mistake probability based on legal-move count and candidate closeness.
- Kept Maximum mode on the engine's strongest single-candidate path.

## 0.3.5

- Added stateful Black-side turn bootstrap for boards without explicit turn labels.
- Combined page hints, initial-position rules, opening activity, player-side detection, and board deltas.
- Preserved message-free direct controller architecture.

## 0.3.4

- Removed background service-worker and Chrome message-bus dependencies.
- Bundled engine execution into the injected isolated controller.

## 0.3.3

- Replaced page-control messaging with direct script execution.
- Added stale-controller replacement after extension reloads.

## 0.3.2

- Added automatic local-player side detection.

## 0.3.1

- Added 1m, 3m, 5m, and 10m match presets.
- Added startup hardening and retained manual timing controls.

## 0.3.0

- Added humanized movement and per-move timing variation.

## 0.2.0

- Added visual board reconstruction without requiring FEN.

## 0.1.0

- Initial local Velocity Chess Chrome extension.
