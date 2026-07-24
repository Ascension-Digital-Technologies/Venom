import { engineName, maximumDiscount, taxRate } from "./constants";
import { clamp, percentage, roundCurrency } from "./math";
function tierRate(tier) {
    if (tier === "enterprise")
        return 0.18;
    if (tier === "pro")
        return 0.10;
    return 0;
}
function couponRate(code) {
    const normalized = String(code || "").trim().toUpperCase();
    if (normalized === "VENOM20")
        return 0.20;
    if (normalized === "TYPESCRIPT10")
        return 0.10;
    return 0;
}
function calculateRisk(total, quantity) {
    const exposure = total / 25;
    const volume = Math.max(0, quantity - 5) * 3;
    return Math.round(clamp(exposure + volume, 0, 100));
}
// @venom: input basePrice:number, quantity:number, customerTier?:string, coupon?:string
// @venom: output subtotal:number, discount:number, tax:number, total:number, riskScore:integer, tier:string, parityCheck:string, engine:string
export function calculateQuote(input) {
    const quantity = Math.max(1, Math.trunc(Number(input.quantity) || 1));
    const basePrice = Math.max(0, Number(input.basePrice) || 0);
    const tier = input.customerTier ?? "standard";
    const subtotal = roundCurrency(basePrice * quantity);
    const discountRate = clamp(tierRate(tier) + couponRate(input.coupon), 0, maximumDiscount());
    const discount = percentage(subtotal, discountRate);
    const taxable = roundCurrency(subtotal - discount);
    const tax = percentage(taxable, taxRate());
    const total = roundCurrency(taxable + tax);
    return {
        subtotal,
        discount,
        tax,
        total,
        riskScore: calculateRisk(total, quantity),
        tier,
        parityCheck: quantity % 2 === 0 ? "even" : "odd",
        engine: engineName()
    };
}
export function runtimeIdentity() {
    return {
        protected: true,
        language: "TypeScript",
        features: ["type erasure", "typed contracts", "module closure", "AST planning"]
    };
}
