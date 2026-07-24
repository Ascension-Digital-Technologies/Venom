import fs from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { pathToFileURL } from "node:url";

const root = path.resolve(path.dirname(new URL(import.meta.url).pathname), "../..");
const temp = await fs.mkdtemp(path.join(os.tmpdir(), "venom-react-package-"));
const nodeModules = path.join(temp, "node_modules");
await fs.mkdir(path.join(nodeModules, "@venom", "react", "src"), { recursive: true });
await fs.mkdir(path.join(nodeModules, "@venom", "runtime", "src"), { recursive: true });
await fs.mkdir(path.join(nodeModules, "react"), { recursive: true });
await fs.copyFile(path.join(root, "packages/react/package.json"), path.join(nodeModules, "@venom", "react", "package.json"));
await fs.copyFile(path.join(root, "packages/react/src/index.js"), path.join(nodeModules, "@venom", "react", "src/index.js"));
await fs.writeFile(path.join(nodeModules, "@venom", "runtime", "package.json"), JSON.stringify({ name: "@venom/runtime", type: "module", exports: "./src/index.js" }));
await fs.writeFile(path.join(nodeModules, "@venom", "runtime", "src/index.js"), `
export const callProtected = async () => null;
export const getRuntimeStatus = () => ({ state: "ready", ready: true, disposed: false, exports: [], transport: "test", lastError: null, apiVersion: 1 });
export const initializeVenom = async () => ({});
export const isVenomRuntimeError = () => false;
export const preloadProtected = async names => ({ ready: names });
`);
await fs.writeFile(path.join(nodeModules, "react", "package.json"), JSON.stringify({ name: "react", type: "module", exports: "./index.js" }));
await fs.writeFile(path.join(nodeModules, "react", "index.js"), `
export const useCallback = value => value;
export const useEffect = () => {};
export const useMemo = value => value();
export const useRef = value => ({ current: value });
export const useState = value => [typeof value === "function" ? value() : value, () => {}];
export const useSyncExternalStore = (_subscribe, snapshot) => snapshot();
`);
const entry = path.join(temp, "import.mjs");
await fs.writeFile(entry, `import { createProtectedHooks } from "@venom/react"; const hooks = createProtectedHooks(["quote", "risk"]); if (hooks.exports.join(",") !== "quote,risk") throw new Error("unexpected exports");`);
await import(pathToFileURL(entry));
await fs.rm(temp, { recursive: true, force: true });
console.log("React package import smoke: PASS");
