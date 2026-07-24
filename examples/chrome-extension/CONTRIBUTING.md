# Contributing

Thank you for helping improve Velocity Chess Browser Engine.

## Before opening an issue

- Confirm the website is one you own or are authorized to automate.
- Reload the unpacked extension and refresh the target page.
- Run `npm run verify` and include the result.
- Check whether the board is DOM-rendered or canvas-only.

## Development workflow

1. Fork the repository and create a focused branch.
2. Keep visual-board detection separate from engine logic.
3. Add a regression test for every compatibility or turn-tracking fix.
4. Run `npm run verify`.
5. Describe the board markup and expected behavior in the pull request.

## Pull request expectations

- No remote code or telemetry.
- No broad host permissions.
- No hidden network dependency.
- No weakening of `allowed_sites.txt` validation.
- No automation targeting websites without authorization.
- New board detectors must be deterministic and covered by tests.
- Humanization changes should remain independent from engine move selection.

## Reporting a board compatibility issue

Include:

- browser and version;
- extension version;
- whether the board is flipped;
- a minimal sanitized HTML fixture, when possible;
- square and piece class/attribute examples;
- the exact error shown by the extension.

Do not include private account data, authentication tokens, or proprietary site code.
