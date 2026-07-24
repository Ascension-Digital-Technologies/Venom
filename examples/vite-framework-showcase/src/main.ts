// @venom: browser
import { calculateProtectedScore } from "./protected-score";

const root = document.querySelector<HTMLElement>("#app");
if (!root) throw new Error("app root missing");
root.innerHTML = `<h1>Venom + Vite</h1><p id="status">Running protected calculation…</p>`;
const result = await calculateProtectedScore([4, 8, 15, 16, 23, 42]);
document.querySelector<HTMLElement>("#status")!.textContent = `TurboJS/WASM score: ${result}`;
