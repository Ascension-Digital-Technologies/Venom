import { defineConfig } from "vite";
import venom from "@venom-js/vite";

export default defineConfig(({ mode }) => ({
  plugins: [venom({ profile: "prod" })]
}));
