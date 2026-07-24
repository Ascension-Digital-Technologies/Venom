#!/usr/bin/env node
import { protect, detectProject, diagnose, initProject, resolveVenomBinary, setupCompiler, writeToolchainLock, dev } from "./index.js";

const args = process.argv.slice(2);
const command = args[0] && !args[0].startsWith("-") ? args.shift() : "protect";
const value = name => { const index = args.indexOf(name); return index >= 0 ? args[index + 1] : undefined; };
const has = name => args.includes(name);

if (has("--help") || has("-h")) {
  console.log(`Usage: venom [protect|dev|detect|doctor|init|setup|bin|lock] [options]\n\nOptions:\n  --root <dir>       Project root (default: current directory)\n  --input <dir>      Compiled input override\n  --out <dir>        Protected output (default: dist-venom)\n  --profile <name>   Venom profile (default: prod)\n  --venom <path>     Venom executable\n  --source <dir>      Venom source checkout for setup\n  --build-dir <dir>   Compiler build directory for setup\n  --config <name>     Compiler configuration for setup\n  --cmake <path>      CMake executable for setup\n  --no-build         Skip the framework build command\n  --force            Replace existing generated config/script\n  --interval <ms>    Dev-mode change polling interval (default: 750)
  --debounce <ms>    Dev-mode settle delay (default: 250)
  --once             Run one dev-mode build and exit
  --quiet            Reduce command output`);
  process.exit(0);
}

const options = {
  root: value("--root"), input: value("--input"), outDir: value("--out"), profile: value("--profile"), venom: value("--venom"),
  source: value("--source"), buildDir: value("--build-dir"), configName: value("--config"), cmake: value("--cmake"),
  build: !has("--no-build"), force: has("--force"), quiet: has("--quiet"),
  interval: value("--interval"), debounce: value("--debounce"), once: has("--once")
};

try {
  if (command === "detect") console.log(JSON.stringify(await detectProject(options.root), null, 2));
  else if (command === "bin") {
    const project = await detectProject(options.root);
    const binary = await resolveVenomBinary({ root: project.root, venom: options.venom, config: project.config });
    if (binary.source === "unresolved") throw new Error("Venom executable was not found locally or on PATH.");
    console.log(binary.command);
  }
  else if (command === "lock") {
    const project = await detectProject(options.root);
    const binary = await resolveVenomBinary({ root: project.root, venom: options.venom, config: project.config });
    if (binary.source === "unresolved") throw new Error("Venom executable was not found locally or on PATH.");
    const result = await writeToolchainLock(project.root, binary.command);
    console.log(`[venom] locked ${result.value.version} at ${result.file}`);
  }
  else if (command === "doctor") {
    const report = await diagnose(options);
    console.log(`[venom] detected ${report.project.framework} project at ${report.project.root}`);
    if (report.binary && report.binary.source !== "unresolved") console.log(`[venom] compiler: ${report.binary.command} (${report.binary.source})`);
    if (!report.findings.length) console.log("[venom] project is ready for zero-config protection");
    for (const finding of report.findings) console.log(`[venom] ${finding.level}: ${finding.message}`);
    if (!report.ok) process.exitCode = 1;
  } else if (command === "init") {
    const result = await initProject(options);
    console.log(`[venom] wrote ${result.configFile}`);
    if (result.packageUpdated) console.log("[venom] added package script: build:protected");
  } else if (command === "setup") {
    const result = await setupCompiler(options);
    console.log(`[venom] compiler built at ${result.binary}`);
    console.log(`[venom] updated ${result.configFile}`);
  } else if (command === "dev") {
    await dev(options);
  } else if (command === "protect") {
    const result = await protect(options);
    console.log(`[venom] protected ${result.framework} project ready at ${result.outDir}`);
  } else throw new Error(`Unknown command: ${command}`);
} catch (error) {
  console.error(`[venom] ${error instanceof Error ? error.message : String(error)}`);
  process.exit(1);
}
