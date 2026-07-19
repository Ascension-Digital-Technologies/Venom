# @venom-js/vite

First-class Vite integration for Venom. Vite owns framework transforms and HMR; Venom performs the protected QuickJS/WASM distribution build after the application graph is ready.

```js
import { defineConfig } from "vite";
import { venom } from "@venom-js/vite";

export default defineConfig({
  plugins: [venom({ site: ".", outDir: "dist-venom", profile: "prod" })]
});
```

Applications may import `virtual:venom-status`, or request `/__venom/status` during development.

## Runtime SDK

Framework applications should use `@venom-js/runtime` or the generated `venom-client.js` module rather than calling generated globals or worker internals directly.
