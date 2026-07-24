# Vendored TurboJS subset

This project vendors only the files needed to embed TurboJS as a static library.
The uploaded TurboJS archive contains many CLI, docs, test, fuzz, and example files that are intentionally not copied.

Included core files:

- `turbojs.c`, `turbojs.h`
- `turbojs-libc.c`, `turbojs-libc.h`
- `dtoa.c`, `dtoa.h`
- `libregexp.c`, `libregexp.h`, `libregexp-opcode.h`
- `libunicode.c`, `libunicode.h`, `libunicode-table.h`
- `cutils.h`, `list.h`
- `turbojs-opcode.h`, `turbojs-atom.h`, `turbojs-c-atomics.h`
- generated builtin headers required by this TurboJS tree

Note: this TurboJS tree does not include `cutils.c`; its CMake build only compiles `dtoa.c`, `libregexp.c`, `libunicode.c`, `turbojs.c`, and optionally `turbojs-libc.c`.
