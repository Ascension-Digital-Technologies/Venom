import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import venom from "@venom/vite";

export default defineConfig({
  resolve: {
    // Local file: dependencies are symlinked during repository development.
    // Preserve their package path so peer packages resolve from this example's node_modules.
    preserveSymlinks: true
  },
  plugins: [
    react(),
    venom({ outDir: "dist-venom", mode: "auto" })
  ],
  build: {
    outDir: "dist",
    manifest: true,
    sourcemap: false
  }
});
