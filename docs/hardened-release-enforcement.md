# v0.88.0 Hardened release enforcement

v0.88.0 tightens browser-protect and hardened release output around fail-closed upstream QuickJS/WASM execution.

## Enforced in hardened/browser-protect builds

- QuickJS/WASM is the only script execution backend.
- Host JavaScript fallback is denied.
- `new Function` source execution hooks are stripped from emitted hardened JS assets.
- `eval(` tokens are stripped from emitted hardened JS assets.
- The runtime must report `quickjs_host_js_fallback_allowed: no`.
- The release gate must report `quickjs_release_fail_closed: yes`.
- The embedded QuickJS WASM runtime must report `venom_qjs_engine_version=83`.
- Hardened wasm bridge assets use compact stems such as `r.<hash>.js` instead of `runtime-js-bridge.<hash>.js`.
- QuickJS script records are kept as opaque `VQJSBC03` native object records and passed into the WASM boundary.

The full package decryption path is still performed by the browser package loader, but native bytecode validation/execution is fail-closed at the QuickJS/WASM boundary. The next deeper hardening pass should move more package section decode into the WASM-owned path.
