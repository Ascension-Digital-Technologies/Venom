# JavaScript API

## `venom.ready()`

Returns a Promise that resolves when the loader, worker, package, decoder, and QuickJS runtime are ready.

## `venom.call(name, value)`

Invokes a protected export by public name. Returns a Promise.

## `venom.exports.<name>(value)`

Convenience facade for a declared protected export.

## Data contract

Arguments and results must be JSON-safe. Functions, DOM nodes, cyclic values, symbols, and host objects are not valid bridge values. Production errors are sanitized and should be handled as application-level failures.
