import { spawn } from "node:child_process";
import { promises as fs } from "node:fs";
import path from "node:path";
import crypto from "node:crypto";

const VIRTUAL_ID = "virtual:venom-status";
const RESOLVED_VIRTUAL_ID = "\0" + VIRTUAL_ID;

function normalizeOptions(user = {}) {
  const root = path.resolve(user.root ?? process.cwd());
  return {
    root,
    venom: user.venom ?? process.env.VENOM_BIN ?? "venom",
    site: path.resolve(root, user.site ?? "."),
    outDir: path.resolve(root, user.outDir ?? "dist-venom"),
    profile: "prod",
    enabled: user.enabled !== false,
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

async function digestTree(root) {
  const hash = crypto.createHash("sha256");
  async function visit(dir) {
    const entries = await fs.readdir(dir, { withFileTypes: true }).catch(() => []);
    entries.sort((a, b) => a.name.localeCompare(b.name));
    for (const entry of entries) {
      if (["node_modules", ".git", ".venom", ".venom-cache"].includes(entry.name)) continue;
      const file = path.join(dir, entry.name);
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

export function venom(userOptions = {}) {
  const options = normalizeOptions(userOptions);
  let viteConfig;
  let building = null;
  let lastDigest = "";
  let timer = null;
  let status = { state: "idle", build: 0, durationMs: 0, error: null, output: options.outDir };

  async function build(reason, force = false) {
    if (!options.enabled) return status;
    if (building) return building;
    building = (async () => {
      const nextDigest = await digestTree(options.site);
      if (!force && nextDigest === lastDigest) return status;
      const started = Date.now();
      status = { ...status, state: "building", error: null };
      try {
        await run(options.venom, ["build", options.site, "--out", options.outDir, "--profile", options.profile, "--quiet", ...options.extraArgs], options.root, viteConfig?.logger);
        lastDigest = nextDigest;
        status = { state: "ready", build: status.build + 1, durationMs: Date.now() - started, error: null, output: options.outDir, reason };
        viteConfig?.logger?.info(`[venom] protected build ready in ${status.durationMs}ms (${reason})`);
      } catch (error) {
        status = { ...status, state: "error", durationMs: Date.now() - started, error: error instanceof Error ? error.message : String(error), reason };
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
    configResolved(config) { viteConfig = config; },
    resolveId(id) { return id === VIRTUAL_ID ? RESOLVED_VIRTUAL_ID : null; },
    load(id) {
      if (id !== RESOLVED_VIRTUAL_ID) return null;
      return `export const status = ${JSON.stringify(status)}; export default status;`;
    },
    async buildStart() {
      if (options.buildOnStart) await build("vite-build-start", true);
    },
    handleHotUpdate(ctx) {
      if (!options.buildOnHotUpdate) return;
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
    async closeBundle() { await build("vite-close-bundle"); },
    api: { build, getStatus: () => ({ ...status }), options }
  };
}

export default venom;
