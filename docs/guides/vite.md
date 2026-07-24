# Vite integration

Venom integrates after Vite's framework transformation stage. React, Vue, Svelte and other Vite ecosystems retain their ordinary development pipeline, while Venom builds the protected distribution from the project source tree.

## Install

```bash
npm install --save-dev @venom/vite
```

## Configuration

```js
import { defineConfig } from "vite";
import venom from "@venom/vite";

export default defineConfig({
  plugins: [venom({
    site: ".",
    outDir: "dist-venom",
    profile: "prod"
  })]
});
```

The plugin provides content-digest incremental rebuilds, serialized compiler execution, a debounced hot-update path, `virtual:venom-status`, and the development status endpoint `/__venom/status`.
