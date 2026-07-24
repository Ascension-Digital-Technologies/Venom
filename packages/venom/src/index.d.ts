export type VenomFramework = "website" | "chrome-extension" | "vite" | "react" | "vue" | "svelte";
export interface VenomConfig {
  $schema?: string;
  input?: string;
  output?: string;
  profile?: string;
  build?: boolean;
  buildScript?: string;
  venom?: string;
  args?: string[];
}
export interface DetectedVenomProject {
  root: string;
  framework: VenomFramework;
  packageManager: "npm" | "pnpm" | "yarn" | "bun";
  packageJson: Record<string, unknown>;
  buildScript: { command: string; args: string[] } | null;
  buildScriptName: string;
  output: string;
  viteConfig: string | null;
  configFile: string | null;
  config: VenomConfig;
}
export interface ProtectOptions {
  root?: string;
  venom?: string;
  input?: string;
  outDir?: string;
  profile?: string;
  build?: boolean;
  quiet?: boolean;
  extraArgs?: string[];
}
export interface DiagnosticFinding { level: "warning" | "error"; code: string; message: string; }
export interface ResolvedVenomBinary { command: string; source: "cli" | "config" | "environment" | "project" | "path" | "unresolved"; }
export function detectProject(input?: string): Promise<DetectedVenomProject>;
export function resolveVenomBinary(options?: { root?: string; venom?: string; config?: VenomConfig }): Promise<ResolvedVenomBinary>;
export interface SetupCompilerOptions { root?: string; source?: string; buildDir?: string; configName?: string; cmake?: string; quiet?: boolean; }
export function setupCompiler(options?: SetupCompilerOptions): Promise<{ projectRoot: string; sourceRoot: string; buildDir: string; binary: string; configFile: string; config: VenomConfig }>;
export function diagnose(options?: { root?: string; venom?: string }): Promise<{ project: DetectedVenomProject; binary: ResolvedVenomBinary | null; findings: DiagnosticFinding[]; ok: boolean }>;
export function initProject(options?: { root?: string; force?: boolean }): Promise<DetectedVenomProject & { configFile: string; config: VenomConfig; packageUpdated: boolean }>;
export function protect(options?: ProtectOptions): Promise<DetectedVenomProject & { input: string; outDir: string; profile: string; venomBinary: string; venomBinarySource: ResolvedVenomBinary["source"] }>;
export interface DevOptions extends ProtectOptions { interval?: number; debounce?: number; once?: boolean; }
export function dev(options?: DevOptions): Promise<{ root: string; buildCount: number; lastError: unknown }>;
export default protect;

export function writeToolchainLock(root: string, binary: string): Promise<{ file: string; value: { schema: number; binary: string; version: string; sha256: string } }>;
export function verifyToolchainLock(root: string, binary: string): Promise<{ present: boolean; ok: boolean; file: string; value: Record<string, unknown> | null; actualHash?: string; actualVersion?: string }>;
