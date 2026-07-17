// @venom: browser
import { scoreRisk } from "./risk";
const React = { createElement: (...args: unknown[]) => args };
export function App() { return <main data-score={scoreRisk([3, 5, 8])}>React qualification</main>; }
