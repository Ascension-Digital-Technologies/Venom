# fpCollect binding collision hotfix

The browser host shim can expose `fpCollect` for sites that call it directly when the original helper is absent. A real site package can also contain a script that declares `const fpCollect = ...`. When host-compatible fallback builds a strict `Function` wrapper with `fpCollect` as a formal parameter, that lexical declaration throws `Identifier 'fpCollect' has already been declared`.

The hotfix filters injected host binding names out of the wrapper parameter list when the chunk source declares the same lexical binding with `const`, `let`, `class`, or `function`. This preserves the missing-global shim for scripts that need it, while allowing real helper scripts such as `fpCollect.min.js` to define their own binding.
