import { defineConfig } from "vite";
import venom from "@venom/vite";

export default defineConfig(({ mode }) => ({
  plugins: [venom({ profile: "prod" })]
}));
