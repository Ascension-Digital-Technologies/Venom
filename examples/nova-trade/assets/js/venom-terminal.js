// @venom: browser
import { assessOrder, analyzePortfolio, generateSignal } from "../../protected/terminal-engine.js";

const fmt = new Intl.NumberFormat("en-US", { maximumFractionDigits: 2 });
let protectionBadge;

function ensureProtectionBadge() {
  if (protectionBadge) return protectionBadge;
  const host = document.querySelector(".top-bar-right") || document.querySelector(".top-bar");
  protectionBadge = document.createElement("div");
  protectionBadge.id = "venomProtectionStatus";
  protectionBadge.className = "account-dropdown";
  protectionBadge.title = "Risk, order validation, and market-signal logic execute in Venom TurboJS/WASM";
  protectionBadge.innerHTML = '<span>VENOM</span><strong style="color:var(--purple)">Protected engine loading…</strong>';
  host?.prepend(protectionBadge);
  return protectionBadge;
}

async function protectedSnapshot() {
  const positions = Array.isArray(globalThis.state?.positions) ? globalThis.state.positions : [];
  const price = Number(globalThis.state?.currentPrice || 0);
  const equity = Number(globalThis.state?.equity || 0);
  const result = await analyzePortfolio({
    equity,
    markPrice: price,
    positions: positions.map((p) => ({
      side: String(p.side || "Long"),
      size: Number(p.size || p.qty || 0),
      entry: Number(p.entry || p.entryPrice || price),
      mark: Number(p.mark || price)
    }))
  });
  const badge = ensureProtectionBadge();
  badge.innerHTML = `<span>VENOM</span><strong style="color:${result.riskLevel === "high" ? "var(--red)" : result.riskLevel === "medium" ? "var(--yellow)" : "var(--green)"}">Risk ${result.riskScore}/100 · ${fmt.format(result.netExposure)}</strong>`;
  return result;
}

async function protectedSignal() {
  const candles = Array.isArray(globalThis.state?.candles) ? globalThis.state.candles.slice(-80) : [];
  if (candles.length < 12) return;
  const result = await generateSignal({
    symbol: String(globalThis.state?.selectedSymbol || "BTC/USDT"),
    candles: candles.map((c) => ({ close: Number(c.close), volume: Number(c.volume || 0) }))
  });
  const overlay = document.querySelector(".chart-overlay-info");
  if (overlay) overlay.dataset.venomSignal = `${result.signal} (${result.confidence}%)`;
}

function installOrderGuard() {
  const original = globalThis.submitPaperOrder;
  if (typeof original !== "function" || original.__venomProtected) return;
  async function guardedSubmit(side, override = null) {
    const priceInput = document.querySelector('.oe-input input[id*="price" i]');
    const sizeInput = document.querySelector('.oe-input input[id*="amount" i], .oe-input input[id*="size" i], .oe-input input[id*="quantity" i]');
    const request = {
      side: String(side || ""),
      orderType: String(globalThis.state?.orderType || "Market"),
      price: Number(override?.price ?? priceInput?.value ?? globalThis.state?.currentPrice ?? 0),
      quantity: Number(override?.quantity ?? override?.size ?? sizeInput?.value ?? 0),
      equity: Number(globalThis.state?.equity || 0),
      currentPrice: Number(globalThis.state?.currentPrice || 0),
      openPositions: Array.isArray(globalThis.state?.positions) ? globalThis.state.positions.length : 0
    };
    const decision = await assessOrder(request);
    if (!decision.approved) {
      globalThis.showToast?.("error", "Protected risk engine", decision.reason);
      return false;
    }
    globalThis.showToast?.("info", "Venom approved", `Risk ${decision.riskScore}/100 · margin $${fmt.format(decision.estimatedMargin)}`);
    return original.call(this, side, override);
  }
  guardedSubmit.__venomProtected = true;
  globalThis.submitPaperOrder = guardedSubmit;
}

async function bootProtectedTerminal() {
  ensureProtectionBadge();
  await globalThis.venom?.ready?.();
  installOrderGuard();
  await protectedSnapshot();
  await protectedSignal();
  setInterval(() => { void protectedSnapshot(); }, 2500);
  setInterval(() => { void protectedSignal(); }, 5000);
}

void bootProtectedTerminal().catch((error) => {
  const badge = ensureProtectionBadge();
  badge.innerHTML = '<span>VENOM</span><strong style="color:var(--red)">Protected engine unavailable</strong>';
  console.error("[nova-trade] protected engine failed", error);
});
