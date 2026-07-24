import { spawn } from "node:child_process";
import { promises as fs } from "node:fs";
import path from "node:path";
import crypto from "node:crypto";

const VIRTUAL_ID = "virtual:venom-status";
const RESOLVED_VIRTUAL_ID = "\0" + VIRTUAL_ID;

function executableNames() {
  return process.platform === "win32"
    ? ["venom.exe", "venom.cmd", "venom.bat", "venom"]
    : ["venom"];
}

async function isExecutable(file) {
  const stat = await fs.stat(file).catch(() => null);
  if (!stat?.isFile()) return false;
  if (process.platform === "win32") return true;
  try {
    await fs.access(file, 1);
    return true;
  } catch {
    return false;
  }
}

function ancestors(start) {
  const result = [];
  let current = path.resolve(start);
  while (true) {
    result.push(current);
    const parent = path.dirname(current);
    if (parent === current) return result;
    current = parent;
  }
}

async function discoverVenomBinary(root, configured) {
  if (configured) {
    const explicit = path.isAbsolute(configured) ? configured : path.resolve(root, configured);
    if (await isExecutable(explicit)) return explicit;
    if (configured.includes("/") || configured.includes("\\")) {
      throw new Error(`Configured Venom executable does not exist: ${explicit}`);
    }
    return configured;
  }

  const candidates = [];
  for (const base of [...new Set([...ancestors(root), ...ancestors(process.cwd())])]) {
    for (const name of executableNames()) {
      candidates.push(path.join(base, "node_modules", ".bin", name));
      candidates.push(path.join(base, "bin", name));
      candidates.push(path.join(base, "build", name));
      candidates.push(path.join(base, "build", "Release", name));
      candidates.push(path.join(base, "build", "RelWithDebInfo", name));
      candidates.push(path.join(base, "build", "full-release", name));
      candidates.push(path.join(base, "build", "release", name));
    }
  }
  for (const candidate of candidates) {
    if (await isExecutable(candidate)) return candidate;
  }
  return "venom";
}

function normalizeOptions(user = {}) {
  const root = path.resolve(user.root ?? process.cwd());
  return {
    root,
    venom: user.venom ?? process.env.VENOM_BIN ?? null,
    site: path.resolve(root, user.site ?? "."),
    outDir: path.resolve(root, user.outDir ?? "dist-venom"),
    profile: "prod",
    mode: user.mode ?? "auto",
    enabled: user.enabled !== false && process.env.VENOM_SKIP_VITE_PLUGIN !== "1",
    buildOnStart: user.buildOnStart !== false,
    buildOnHotUpdate: user.buildOnHotUpdate !== false,
    debounceMs: Number.isFinite(user.debounceMs) ? Math.max(0, user.debounceMs) : 120,
    extraArgs: Array.isArray(user.extraArgs) ? [...user.extraArgs] : [],
    logLevel: user.logLevel ?? "info"
  };
}

function run(command, args, cwd, logger) {
  return new Promise((resolve, reject) => {
    const child = spawn(command, args, { cwd, stdio: ["ignore", "pipe", "pipe"], shell: false });
    let stdout = "";
    let stderr = "";
    child.stdout.on("data", chunk => { stdout += chunk; if (logger) logger.info(chunk.toString().trimEnd()); });
    child.stderr.on("data", chunk => { stderr += chunk; if (logger) logger.warn(chunk.toString().trimEnd()); });
    child.on("error", reject);
    child.on("close", code => {
      if (code === 0) resolve({ stdout, stderr });
      else reject(new Error(`Venom exited with code ${code}\n${stderr || stdout}`));
    });
  });
}

async function digestTree(root, ignored = []) {
  const hash = crypto.createHash("sha256");
  const ignoredRoots = ignored.map(value => path.resolve(value));
  async function visit(dir) {
    if (ignoredRoots.some(value => dir === value || dir.startsWith(value + path.sep))) return;
    const entries = await fs.readdir(dir, { withFileTypes: true }).catch(() => []);
    entries.sort((a, b) => a.name.localeCompare(b.name));
    for (const entry of entries) {
      if (["node_modules", ".git", ".venom", ".venom-cache"].includes(entry.name)) continue;
      const file = path.join(dir, entry.name);
      if (ignoredRoots.some(value => file === value || file.startsWith(value + path.sep))) continue;
      const rel = path.relative(root, file).replaceAll(path.sep, "/");
      if (entry.isDirectory()) await visit(file);
      else {
        hash.update(rel);
        hash.update(await fs.readFile(file));
      }
    }
  }
  await visit(root);
  return hash.digest("hex");
}

async function detectFramework(root) {
  try {
    const pkg = JSON.parse(await fs.readFile(path.join(root, "package.json"), "utf8"));
    const deps = { ...(pkg.dependencies ?? {}), ...(pkg.devDependencies ?? {}) };
    if (deps.react && deps["react-dom"]) return "react";
    if (deps.vue) return "vue";
    if (deps.svelte) return "svelte";
  } catch {}
  return "vite";
}

export function venom(userOptions = {}) {
  const options = normalizeOptions(userOptions);
  let viteConfig;
  let building = null;
  let lastDigest = "";
  let timer = null;
  let framework = "vite";
  let viteOutput = null;
  let status = { state: "idle", build: 0, durationMs: 0, error: null, output: options.outDir, framework };

  function useViteOutput() {
    if (options.mode === "vite-output") return true;
    if (options.mode === "source") return false;
    return viteConfig?.command === "build";
  }

  async function build(reason, force = false, siteOverride = null) {
    if (!options.enabled) return status;
    if (building) return building;
    building = (async () => {
      const site = path.resolve(siteOverride ?? options.site);
      const stat = await fs.stat(site).catch(() => null);
      if (!stat?.isDirectory()) throw new Error(`Venom input directory does not exist: ${site}`);
      const nextDigest = await digestTree(site, [options.outDir]);
      if (!force && nextDigest === lastDigest) return status;
      const started = Date.now();
      status = { ...status, state: "building", error: null, framework, input: site };
      try {
        await run(options.venom, ["build", site, "--out", options.outDir, "--profile", options.profile, "--quiet", ...options.extraArgs], options.root, viteConfig?.logger);
        lastDigest = nextDigest;
        status = { state: "ready", build: status.build + 1, durationMs: Date.now() - started, error: null, output: options.outDir, reason, framework, input: site };
        viteConfig?.logger?.info(`[venom] protected ${framework} build ready in ${status.durationMs}ms (${reason})`);
      } catch (error) {
        status = { ...status, state: "error", durationMs: Date.now() - started, error: error instanceof Error ? error.message : String(error), reason, framework, input: site };
        throw error;
      }
      return status;
    })();
    const active = building;
    try {
      return await active;
    } finally {
      if (building === active) building = null;
    }
  }

  function schedule(reason) {
    clearTimeout(timer);
    timer = setTimeout(() => build(reason).catch(error => viteConfig?.logger?.error(error.message)), options.debounceMs);
  }

  return {
    name: "venom",
    enforce: "post",
    async configResolved(config) {
      viteConfig = config;
      framework = await detectFramework(options.root);
      viteOutput = path.resolve(config.root ?? options.root, config.build?.outDir ?? "dist");
      options.venom = await discoverVenomBinary(options.root, options.venom);
      status = { ...status, framework };
      if (framework === "react") config.logger.info("[venom] React detected; production protection will ingest Vite's compiled output");
    },
    resolveId(id) { return id === VIRTUAL_ID ? RESOLVED_VIRTUAL_ID : null; },
    load(id) {
      if (id !== RESOLVED_VIRTUAL_ID) return null;
      return `export const status = ${JSON.stringify(status)}; export default status;`;
    },
    async buildStart() {
      if (options.buildOnStart && !useViteOutput()) await build("vite-build-start", true);
    },
    handleHotUpdate(ctx) {
      if (!options.buildOnHotUpdate || useViteOutput()) return;
      const normalized = path.resolve(ctx.file);
      if (normalized.startsWith(options.site + path.sep) || normalized === options.site) schedule("vite-hot-update");
    },
    configureServer(server) {
      server.middlewares.use("/__venom/status", (_req, res) => {
        res.statusCode = status.state === "error" ? 500 : 200;
        res.setHeader("Content-Type", "application/json; charset=utf-8");
        res.setHeader("Cache-Control", "no-store");
        res.end(JSON.stringify(status));
      });
    },
    async writeBundle() {
      if (useViteOutput()) await build("vite-write-bundle", true, viteOutput);
    },
    async closeBundle() {
      if (!useViteOutput()) await build("vite-close-bundle");
    },
    api: { build, getStatus: () => ({ ...status }), options }
  };
}

export default venom;
