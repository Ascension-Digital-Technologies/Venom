import { StrictMode, useMemo, useState } from "react";
import { createRoot } from "react-dom/client";
import { useVenomRuntime } from "@venom/react";
import "./style.css";

type Quote = { seats: number; annual: boolean };

function calculateQuote({ seats, annual }: Quote): number {
  const monthly = Math.max(1, seats) * 18;
  return annual ? Math.round(monthly * 12 * 0.82) : monthly;
}

function App() {
  const runtime = useVenomRuntime({ timeout: 30000 });
  const [seats, setSeats] = useState(5);
  const [annual, setAnnual] = useState(true);
  const total = useMemo(() => calculateQuote({ seats, annual }), [seats, annual]);
  return (
    <main>
      <p className="eyebrow">React + Vite + Venom · runtime {runtime.ready ? "ready" : runtime.state}</p>
      <h1>Protected React production output</h1>
      <label>Seats <input aria-label="Seats" type="number" min="1" value={seats} onChange={event => setSeats(Number(event.target.value))} /></label>
      <label><input type="checkbox" checked={annual} onChange={event => setAnnual(event.target.checked)} /> Annual billing</label>
      <output>${total} {annual ? "per year" : "per month"}</output>
    </main>
  );
}

const root = document.getElementById("root");
if (!root) throw new Error("React root is missing");
createRoot(root).render(<StrictMode><App /></StrictMode>);
