// @venom: browser
import { calculateProtectedScore } from "./protected-score";

async function main(): Promise<void> {
  const root = document.querySelector<HTMLElement>("#app");
  if (!root) throw new Error("app root missing");

  root.innerHTML = `<h1>Venom + Vite</h1><p id="status">Running protected calculation…</p>`;

  const result = await calculateProtectedScore([4, 8, 15, 16, 23, 42]);
  const status = document.querySelector<HTMLElement>("#status");
  if (!status) throw new Error("status element missing");
  status.textContent = `TurboJS/WASM score: ${result}`;
}

void main().catch((error: unknown) => {
  const message = error instanceof Error ? error.message : String(error);
  const status = document.querySelector<HTMLElement>("#status");
  if (status) status.textContent = `Protected calculation failed: ${message}`;
});
