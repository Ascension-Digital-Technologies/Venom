import { calculateQuote, runtimeIdentity } from "../protected/pricing";
import formatMoney from "./format";
const form = document.querySelector("#quote-form");
const output = document.querySelector("#result");
const badge = document.querySelector("#runtime-badge");
function setStatus(status, message) {
    if (!badge)
        return;
    badge.dataset.status = status;
    badge.textContent = message;
}
function render(result) {
    if (!output)
        return;
    output.innerHTML = `
    <div class="metric"><span>Subtotal</span><strong>${formatMoney(result.subtotal)}</strong></div>
    <div class="metric"><span>Discount</span><strong>-${formatMoney(result.discount)}</strong></div>
    <div class="metric"><span>Tax</span><strong>${formatMoney(result.tax)}</strong></div>
    <div class="metric total"><span>Total</span><strong>${formatMoney(result.total)}</strong></div>
    <div class="details">
      <span>Risk ${result.riskScore}/100</span>
      <span>${result.tier} tier</span>
      <span>${result.parityCheck} quantity</span>
    </div>
    <small>${result.engine}</small>`;
}
async function submitQuote(event) {
    event?.preventDefault();
    if (!form)
        return;
    const data = new FormData(form);
    const request = {
        basePrice: Number(data.get("basePrice")),
        quantity: Number(data.get("quantity")),
        customerTier: String(data.get("customerTier") || "standard"),
        coupon: String(data.get("coupon") || "")
    };
    setStatus("loading", "Protected TypeScript calculating…");
    try {
        const result = await calculateQuote(request);
        render(result);
        setStatus("ready", "Protected TypeScript active");
    }
    catch (error) {
        setStatus("error", "Protected runtime failed");
        if (output)
            output.textContent = error instanceof Error ? error.message : String(error);
    }
}
async function boot() {
    await globalThis.venom?.ready?.();
    const identity = await runtimeIdentity();
    setStatus(identity.protected ? "ready" : "error", `${identity.language} AST pipeline verified`);
    form?.addEventListener("submit", (event) => { void submitQuote(event); });
    await submitQuote();
}
void boot().catch((error) => {
    setStatus("error", "Unable to initialize Venom");
    console.error("[typescript-showcase]", error);
});
