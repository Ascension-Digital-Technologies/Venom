# Phase 1G: Dual-format QuickJS bytecode accounting

## Finding

The structural release scanner introduced during pre-release development only inspected `VJSB0006` bundle entries. Protected function registries are emitted as a dedicated JavaScript section whose payload begins directly with `VQJSE006`, so valid protected-chess builds were incorrectly reported as containing zero records.

## Fix

Release accounting now accepts exactly two structural storage forms:

1. a JavaScript section whose payload begins with `VQJSE006`, counted once; or
2. a `VJSB0006` bundle whose bytecode-flagged entry payload begins with `VQJSE006`.

No arbitrary marker scan is used. Integrity and metadata text cannot inflate the count, while direct protected-registry sections are no longer missed.

## Expected regressions

- protected-chess: one intent, one direct registry envelope, one counted record;
- bot-detection: two intents, one direct registry envelope, one counted record;
- whole-file protected script: one bundle entry, one counted record;
- metadata containing `VQJSE006`: zero counted records.
