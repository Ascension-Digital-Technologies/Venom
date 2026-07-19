# Phase 1F: Structural QuickJS Bytecode Accounting

Date: 2026-07-14
Status: Fixed

## Failure

The bot-detection production build emitted one QuickJS registry envelope containing two protected function intents. The release checker reported two QuickJS records even though `protection_closure.expected_quickjs_records` correctly reported one.

The prior checker counted occurrences of the ASCII envelope magic `VQJSE006` across decoded package section bytes. This remained vulnerable to false positives from non-executable data and embedded metadata.

## Fix

Release accounting now parses `VJSB0006` JavaScript bundle tables and counts only entries that:

1. carry the `BYTECODE_ENCODED` script flag;
2. have an in-range code payload; and
3. begin with `VQJSE006` at payload offset zero.

Multiple protected functions compiled into one registry envelope therefore count as one executable QuickJS record, matching the compiler closure model.

## Security effect

The release gate no longer accepts arbitrary marker text as executable bytecode and no longer rejects valid builds because metadata happens to mention the envelope magic.

Malformed bundle entries are not counted. Existing package parser validation and release checks continue to reject malformed protected artifacts through their normal validation paths.

## Validation

- Linux Release compiler build: PASS
- Structural scanner compiled into `venom`: PASS
- Old broad `count_bytes(section.data, "VQJSE006")` release accounting removed: PASS
- Source file size for modified inspection unit remains under its 400-line gate: PASS (366 lines)
- Full source-layout smoke: pre-existing unrelated failure (`src/pipeline/js.cpp` is 519 lines; limit 500)

## Windows regression expectation

For bot-detection:

- protected intents requested: 2
- protected intents resolved: 2
- expected QuickJS records: 1
- emitted QuickJS records: 1
- release status: PASS
