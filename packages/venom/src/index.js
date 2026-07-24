import { spawn } from "node:child_process";
import { createHash } from "node:crypto";
import { promises as fs } from "node:fs";
import path from "node:path";
import process from "node:process";

const CONFIG_NAMES = ["venom.config.json", ".venomrc.json"];

const TOOLCHAIN_LOCK = ".venom/toolchain.lock.json";

async function sha256File(file) {
  const data = await fs.readFile(file);
  return createHash("sha256").update(data).digest("hex");
}

async function probeVenomVersion(command) {
  const result = await run(command, ["--version"], { quiet: true });
  const text = String(result.stdout || result.stderr || "").trim();
  const match = text.match(/(?:venom\s+)?([0-9]+\.[0-9]+\.[0-9]+(?:[-+][^\s]+)?)/i);
  return match ? match[1] : text || "unknown";
}

export async function writeToolchainLock(root, binary) {
  const projectRoot = path.resolve(root || process.cwd());
  const lockFile = path.join(projectRoot, TOOLCHAIN_LOCK);
  await fs.mkdir(path.dirname(lockFile), { recursive: true });
  const value = {
    schema: 1,
    binary: path.relative(projectRoot, binary).replaceAll("\\", "/"),
    version: await probeVenomVersion(binary),
    sha256: await sha256File(binary)
  };
  await writeJson(lockFile, value);
  return { file: lockFile, value };
}

export async function verifyToolchainLock(root, binary) {
  const projectRoot = path.resolve(root || process.cwd());
  const lockFile = path.join(projectRoot, TOOLCHAIN_LOCK);
  if (!(await exists(lockFile))) return { present: false, ok: true, file: lockFile, value: null };
  const value = await readJson(lockFile);
  const actualHash = await sha256File(binary);
  const actualVersion = await probeVenomVersion(binary);
  const ok = value.sha256 === actualHash && value.version === actualVersion;
  return { present: true, ok, file: lockFile, value, actualHash, actualVersion };
}

const FRAMEWORK_OUTPUTS = {
  react: ["dist", "build", "out"],
  vue: ["dist"],
  svelte: ["build", "dist"],
  vite: ["dist"]
};


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

async function firstExecutable(candidates) {
  for (const candidate of candidates) {
    if (candidate && await isExecutable(candidate)) return path.resolve(candidate);
  }
  return null;
}

function pathCandidates(name) {
  const directories = String(process.env.PATH || "").split(path.delimiter).filter(Boolean);
  return directories.map(directory => path.join(directory, name));
}

export async function resolveVenomBinary(userOptions = {}) {
  const root = path.resolve(userOptions.root || process.cwd());
  const explicit = userOptions.venom || userOptions.config?.venom || process.env.VENOM_BIN;
  if (explicit) {
    const resolved = path.isAbsolute(explicit) ? explicit : path.resolve(root, explicit);
    if (await isExecutable(resolved)) return { command: resolved, source: userOptions.venom ? "cli" : userOptions.config?.venom ? "config" : "environment" };
    if (!explicit.includes(path.sep) && !explicit.includes("/")) {
      const found = await firstExecutable(executableNames().flatMap(name => name === explicit ? pathCandidates(name) : []));
      if (found) return { command: found, source: "path" };
    }
    throw new Error(`Configured Venom executable was not found or is not executable: ${resolved}`);
  }

  const localCandidates = [];
  for (const name of executableNames()) {
    localCandidates.push(path.join(root, "node_modules", ".bin", name));
    localCandidates.push(path.join(root, "bin", name));
    localCandidates.push(path.join(root, "build", name));
    localCandidates.push(path.join(root, "build", "Release", name));
    localCandidates.push(path.join(root, "build", "full-release", name));
    localCandidates.push(path.join(root, "build", "release", name));
  }
  const local = await firstExecutable(localCandidates);
  if (local) return { command: local, source: "project" };

  for (const name of executableNames()) {
    const found = await firstExecutable(pathCandidates(name));
    if (found) return { command: found, source: "path" };
  }
  return { command: "venom", source: "unresolved" };
}


async function findBuiltVenom(buildDir) {
  const names = new Set(executableNames());
  const direct = [];
  for (const name of names) {
    direct.push(path.join(buildDir, name));
    direct.push(path.join(buildDir, "Release", name));
    direct.push(path.join(buildDir, "RelWithDebInfo", name));
    direct.push(path.join(buildDir, "MinSizeRel", name));
  }
  const found = await firstExecutable(direct);
  if (found) return found;

  const queue = [{ dir: buildDir, depth: 0 }];
  while (queue.length) {
    const current = queue.shift();
    if (!current || current.depth > 3) continue;
    const entries = await fs.readdir(current.dir, { withFileTypes: true }).catch(() => []);
    for (const entry of entries) {
      const full = path.join(current.dir, entry.name);
      if (entry.isFile() && names.has(entry.name) && await isExecutable(full)) return path.resolve(full);
      if (entry.isDirectory() && !entry.name.startsWith(".")) queue.push({ dir: full, depth: current.depth + 1 });
    }
  }
  return null;
}

async function findVenomSourceRoot(start) {
  let current = path.resolve(start);
  for (;;) {
    if (await exists(path.join(current, "CMakeLists.txt")) && await exists(path.join(current, "src", "cli", "main.cpp"))) return current;
    const parent = path.dirname(current);
    if (parent === current) return null;
    current = parent;
  }
}

export async function setupCompiler(userOptions = {}) {
  const projectRoot = path.resolve(userOptions.root || process.cwd());
  const sourceRoot = await findVenomSourceRoot(userOptions.source || projectRoot);
  if (!sourceRoot) {
    throw new Error("Venom source checkout was not found. Run with --source <path-to-venom-source>.");
  }
  const configName = String(userOptions.configName || "Release");
  const buildDir = path.resolve(sourceRoot, userOptions.buildDir || ".venom/toolchain");
  const cmake = String(userOptions.cmake || process.env.CMAKE || "cmake");
  await fs.mkdir(buildDir, { recursive: true });
  await run(cmake, ["-S", sourceRoot, "-B", buildDir, `-DCMAKE_BUILD_TYPE=${configName}`], { cwd: sourceRoot, quiet: !!userOptions.quiet });
  await run(cmake, ["--build", buildDir, "--config", configName, "--target", "venom", "--parallel"], { cwd: sourceRoot, quiet: !!userOptions.quiet });

  const binary = await findBuiltVenom(buildDir);
  if (!binary) throw new Error(`Venom built successfully but no compiler executable was found under ${buildDir}.`);

  const configFile = path.join(projectRoot, "venom.config.json");
  const existing = await exists(configFile) ? await readJson(configFile) : {
    $schema: "https://venom.dev/schemas/venom-config-v1.json",
    output: "dist-venom",
    profile: "prod"
  };
  existing.venom = path.relative(projectRoot, binary).replaceAll("\\", "/");
  await writeJson(configFile, existing);
  const toolchainLock = await writeToolchainLock(projectRoot, binary);
  return { projectRoot, sourceRoot, buildDir, binary, configFile, config: existing, toolchainLock };
}

function commandForPackageManager(manager, script = "build") {
  if (manager === "pnpm") return { command: "pnpm", args: ["run", script] };
  if (manager === "yarn") return { command: "yarn", args: [script] };
  if (manager === "bun") return { command: "bun", args: ["run", script] };
  return { command: "npm", args: ["run", script] };
}

async function exists(file) {
  return !!(await fs.stat(file).catch(() => null));
}

async function readJson(file) {
  try {
    return JSON.parse(await fs.readFile(file, "utf8"));
  } catch (error) {
    throw new Error(`Could not read ${file}: ${error instanceof Error ? error.message : String(error)}`);
  }
}

async function writeJson(file, value) {
  await fs.writeFile(file, `${JSON.stringify(value, null, 2)}\n`, "utf8");
}

async function detectPackageManager(root, pkg = {}) {
  const declared = String(pkg.packageManager || "").split("@")[0];
  if (["npm", "pnpm", "yarn", "bun"].includes(declared)) return declared;
  if (await exists(path.join(root, "pnpm-lock.yaml"))) return "pnpm";
  if (await exists(path.join(root, "yarn.lock"))) return "yarn";
  if (await exists(path.join(root, "bun.lockb")) || await exists(path.join(root, "bun.lock"))) return "bun";
  return "npm";
}

function dependencyMap(pkg = {}) {
  return { ...(pkg.dependencies || {}), ...(pkg.devDependencies || {}) };
}

async function findConfig(root) {
  for (const name of CONFIG_NAMES) {
    const file = path.join(root, name);
    if (await exists(file)) return { file, value: await readJson(file) };
  }
  return { file: null, value: {} };
}

async function findFrameworkOutput(root, framework, configured) {
  if (configured) return path.resolve(root, configured);
  for (const candidate of FRAMEWORK_OUTPUTS[framework] || ["dist"]) {
    const output = path.resolve(root, candidate);
    if (await exists(output)) return output;
  }
  return path.resolve(root, (FRAMEWORK_OUTPUTS[framework] || ["dist"])[0]);
}

export async function detectProject(input = process.cwd()) {
  const root = path.resolve(input);
  const manifestFile = path.join(root, "manifest.json");
  const packageFile = path.join(root, "package.json");
  const hasManifest = await exists(manifestFile);
  const pkg = await exists(packageFile) ? await readJson(packageFile) : {};
  const deps = dependencyMap(pkg);
  const viteNames = ["vite.config.js", "vite.config.mjs", "vite.config.ts", "vite.config.cjs"];
  const viteConfig = (await Promise.all(viteNames.map(async name => (await exists(path.join(root, name))) ? name : null))).find(Boolean);
  const isVite = !!deps.vite || !!viteConfig;
  const framework = deps.react && deps["react-dom"] ? "react" : deps.vue ? "vue" : deps.svelte ? "svelte" : isVite ? "vite" : hasManifest ? "chrome-extension" : "website";
  const packageManager = await detectPackageManager(root, pkg);
  const config = await findConfig(root);
  const buildScriptName = String(config.value.buildScript || "build");
  const buildScript = pkg.scripts?.[buildScriptName] ? commandForPackageManager(packageManager, buildScriptName) : null;
  const output = framework === "chrome-extension"
    ? root
    : await findFrameworkOutput(root, framework, config.value.input);
  return {
    root,
    framework,
    packageManager,
    packageJson: pkg,
    buildScript,
    buildScriptName,
    output,
    viteConfig: viteConfig || null,
    configFile: config.file,
    config: config.value
  };
}

function run(command, args, options = {}) {
  return new Promise((resolve, reject) => {
    const child = spawn(command, args, {
      cwd: options.cwd,
      env: options.env || process.env,
      stdio: options.quiet ? ["ignore", "pipe", "pipe"] : "inherit",
      shell: process.platform === "win32"
    });
    let stdout = "";
    let stderr = "";
    if (options.quiet) {
      child.stdout?.on("data", chunk => { stdout += chunk; });
      child.stderr?.on("data", chunk => { stderr += chunk; });
    }
    child.on("error", error => reject(new Error(`Could not start ${command}: ${error.message}`)));
    child.on("close", code => code === 0 ? resolve({ stdout, stderr }) : reject(new Error(`${command} exited with code ${code}${stderr ? `\n${stderr}` : ""}`)));
  });
}

export async function diagnose(userOptions = {}) {
  const detected = await detectProject(userOptions.root || process.cwd());
  const findings = [];
  let binary;
  try {
    binary = await resolveVenomBinary({ root: detected.root, venom: userOptions.venom, config: detected.config });
    if (binary.source === "unresolved") findings.push({ level: "error", code: "venom-binary-missing", message: "Venom executable was not found locally or on PATH. Build/install Venom or set VENOM_BIN / config.venom." });
    else {
      const lock = await verifyToolchainLock(detected.root, binary.command);
      if (lock.present && !lock.ok) findings.push({ level: "error", code: "toolchain-lock-mismatch", message: `Venom compiler does not match ${lock.file}. Run venom lock or venom setup.` });
    }
  } catch (error) {
    findings.push({ level: "error", code: "venom-binary-invalid", message: error instanceof Error ? error.message : String(error) });
  }
  if (["react", "vue", "svelte", "vite"].includes(detected.framework) && !detected.buildScript) {
    findings.push({ level: "error", code: "missing-build-script", message: `No '${detected.buildScriptName}' package script was found.` });
  }
  if (detected.config.input && !(await exists(detected.output))) {
    findings.push({ level: "warning", code: "configured-input-missing", message: `Configured input does not exist yet: ${detected.output}` });
  }
  if (detected.framework === "website" && !(await exists(path.join(detected.root, "index.html")))) {
    findings.push({ level: "warning", code: "missing-index", message: "No index.html was found; specify an input directory in venom.config.json if this is intentional." });
  }
  return { project: detected, binary: binary || null, findings, ok: !findings.some(item => item.level === "error") };
}

export async function initProject(userOptions = {}) {
  const root = path.resolve(userOptions.root || process.cwd());
  const detected = await detectProject(root);
  const configFile = path.join(root, "venom.config.json");
  if ((await exists(configFile)) && !userOptions.force) {
    throw new Error(`Configuration already exists at ${configFile}. Use --force to replace it.`);
  }
  const config = {
    $schema: "https://venom.dev/schemas/venom-config-v1.json",
    input: detected.framework === "chrome-extension" || detected.framework === "website" ? "." : path.relative(root, detected.output).replaceAll("\\", "/"),
    output: "dist-venom",
    profile: "prod",
    build: detected.framework !== "chrome-extension" && detected.framework !== "website",
    buildScript: detected.buildScriptName || "build"
  };
  await writeJson(configFile, config);

  const packageFile = path.join(root, "package.json");
  let packageUpdated = false;
  if (await exists(packageFile)) {
    const pkg = await readJson(packageFile);
    pkg.scripts ||= {};
    if (!pkg.scripts["build:protected"] || userOptions.force) {
      pkg.scripts["build:protected"] = "venom";
      await writeJson(packageFile, pkg);
      packageUpdated = true;
    }
  }
  return { ...detected, configFile, config, packageUpdated };
}


const DEFAULT_WATCH_IGNORES = new Set([
  ".git", ".venom", ".venom-cache", "node_modules", "dist-venom", "build", ".cache", ".vite"
]);

async function snapshotTree(root, ignoredPaths = []) {
  const ignored = new Set([...DEFAULT_WATCH_IGNORES, ...ignoredPaths.filter(Boolean).map(value => path.basename(value))]);
  const hash = createHash("sha256");
  const queue = [root];
  while (queue.length) {
    const directory = queue.shift();
    const entries = await fs.readdir(directory, { withFileTypes: true }).catch(() => []);
    entries.sort((a, b) => a.name.localeCompare(b.name));
    for (const entry of entries) {
      if (ignored.has(entry.name)) continue;
      const full = path.join(directory, entry.name);
      const relative = path.relative(root, full).replaceAll("\\", "/");
      if (entry.isDirectory()) {
        queue.push(full);
        continue;
      }
      if (!entry.isFile()) continue;
      const stat = await fs.stat(full).catch(() => null);
      if (!stat) continue;
      hash.update(relative);
      hash.update(String(stat.size));
      hash.update(String(Math.trunc(stat.mtimeMs)));
    }
  }
  return hash.digest("hex");
}

function sleep(milliseconds) {
  return new Promise(resolve => setTimeout(resolve, milliseconds));
}

export async function dev(userOptions = {}) {
  const root = path.resolve(userOptions.root || process.cwd());
  const interval = Math.max(200, Number(userOptions.interval || 750));
  const debounce = Math.max(0, Number(userOptions.debounce || 250));
  const once = !!userOptions.once;
  let building = false;
  let pending = false;
  let stopped = false;
  let buildCount = 0;
  let lastError = null;

  const detected = await detectProject(root);
  const ignored = [
    userOptions.outDir || detected.config?.output || "dist-venom",
    detected.config?.input,
    detected.output
  ];

  const build = async reason => {
    if (building) {
      pending = true;
      return;
    }
    building = true;
    try {
      if (!userOptions.quiet) console.log(`[venom] ${reason}; rebuilding protected output`);
      await protect(userOptions);
      buildCount += 1;
      lastError = null;
      if (!userOptions.quiet) console.log(`[venom] protected rebuild ${buildCount} complete`);
    } catch (error) {
      lastError = error;
      console.error(`[venom] rebuild failed: ${error instanceof Error ? error.message : String(error)}`);
    } finally {
      building = false;
      if (pending && !stopped) {
        pending = false;
        await build("changes arrived during the previous build");
      }
    }
  };

  await build("initial build");
  if (once) return { root, buildCount, lastError };

  let previous = await snapshotTree(root, ignored);
  const stop = () => { stopped = true; };
  process.once("SIGINT", stop);
  process.once("SIGTERM", stop);
  if (!userOptions.quiet) console.log(`[venom] watching ${root} (poll ${interval} ms)`);

  while (!stopped) {
    await sleep(interval);
    const current = await snapshotTree(root, ignored);
    if (current === previous) continue;
    previous = current;
    if (debounce) await sleep(debounce);
    const settled = await snapshotTree(root, ignored);
    previous = settled;
    await build("project change detected");
  }
  return { root, buildCount, lastError };
}

export async function protect(userOptions = {}) {
  const detected = await detectProject(userOptions.root || process.cwd());
  const config = detected.config || {};
  const binary = await resolveVenomBinary({ root: detected.root, venom: userOptions.venom, config });
  if (binary.source === "unresolved") {
    throw new Error("Venom executable was not found locally or on PATH. Build/install Venom, set VENOM_BIN, or configure the \"venom\" path in venom.config.json.");
  }
  const venom = binary.command;
  const toolchainLock = await verifyToolchainLock(detected.root, venom);
  if (toolchainLock.present && !toolchainLock.ok) {
    throw new Error(`Venom compiler does not match ${toolchainLock.file}. Run venom lock or venom setup.`);
  }
  const outDir = path.resolve(detected.root, userOptions.outDir || config.output || "dist-venom");
  const configBuild = config.build !== false;
  const shouldBuild = userOptions.build !== false && configBuild && !!detected.buildScript && detected.framework !== "chrome-extension";

  if (shouldBuild) {
    await run(detected.buildScript.command, detected.buildScript.args, {
      cwd: detected.root,
      quiet: !!userOptions.quiet,
      env: { ...process.env, VENOM_SKIP_VITE_PLUGIN: "1" }
    });
  }

  let input = detected.root;
  if (shouldBuild || ["react", "vue", "svelte", "vite"].includes(detected.framework)) {
    input = path.resolve(detected.root, userOptions.input || config.input || path.relative(detected.root, detected.output));
    const stat = await fs.stat(input).catch(() => null);
    if (!stat?.isDirectory()) {
      throw new Error(`Compiled output was not found at ${input}. Run the project build, use --input, or set "input" in venom.config.json.`);
    }
  }

  const profile = userOptions.profile || config.profile || "prod";
  const args = ["build", input, "--out", outDir, "--profile", profile];
  if (userOptions.quiet) args.push("--quiet");
  if (Array.isArray(config.args)) args.push(...config.args.map(String));
  if (Array.isArray(userOptions.extraArgs)) args.push(...userOptions.extraArgs);
  await run(venom, args, { cwd: detected.root, quiet: !!userOptions.quiet });
  return { ...detected, input, outDir, profile, venomBinary: venom, venomBinarySource: binary.source };
}

export default protect;
