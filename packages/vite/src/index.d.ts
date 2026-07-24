import type { Plugin } from "vite";

export interface VenomPluginOptions {
  root?: string;
  venom?: string;
  site?: string;
  outDir?: string;
  profile?: "prod";
  /** auto protects Vite output during production builds; source keeps legacy source-tree ingestion. */
  mode?: "auto" | "source" | "vite-output";
  enabled?: boolean;
  buildOnStart?: boolean;
  buildOnHotUpdate?: boolean;
  debounceMs?: number;
  extraArgs?: string[];
  logLevel?: "silent" | "info" | "debug";
}

export interface VenomBuildStatus {
  state: "idle" | "building" | "ready" | "error";
  build: number;
  durationMs: number;
  error: string | null;
  output: string;
  framework?: "vite" | "react" | "vue" | "svelte";
  input?: string;
  reason?: string;
}

export interface VenomVitePlugin extends Plugin {
  api: {
    build(reason: string, force?: boolean): Promise<VenomBuildStatus>;
    getStatus(): VenomBuildStatus;
    options: Readonly<VenomPluginOptions>;
  };
}

export function venom(options?: VenomPluginOptions): VenomVitePlugin;
export default venom;
