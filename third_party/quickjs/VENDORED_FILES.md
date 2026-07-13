# Vendored QuickJS subset

This project vendors only the files needed to embed QuickJS as a static library.
The uploaded QuickJS archive contains many CLI, docs, test, fuzz, and example files that are intentionally not copied.

Included core files:

- `quickjs.c`, `quickjs.h`
- `quickjs-libc.c`, `quickjs-libc.h`
- `dtoa.c`, `dtoa.h`
- `libregexp.c`, `libregexp.h`, `libregexp-opcode.h`
- `libunicode.c`, `libunicode.h`, `libunicode-table.h`
- `cutils.h`, `list.h`
- `quickjs-opcode.h`, `quickjs-atom.h`, `quickjs-c-atomics.h`
- generated builtin headers required by this QuickJS tree

Note: this QuickJS tree does not include `cutils.c`; its CMake build only compiles `dtoa.c`, `libregexp.c`, `libunicode.c`, `quickjs.c`, and optionally `quickjs-libc.c`.
