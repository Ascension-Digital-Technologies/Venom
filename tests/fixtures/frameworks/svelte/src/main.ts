// @venom: browser
import { calculate } from "./engine";
export function render(node: HTMLElement) { node.textContent = String(calculate(6, 7)); }
