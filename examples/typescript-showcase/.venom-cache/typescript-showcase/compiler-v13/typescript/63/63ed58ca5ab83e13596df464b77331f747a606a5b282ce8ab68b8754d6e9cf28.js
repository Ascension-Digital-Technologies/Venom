// @venom: protected module
export function clamp(value, minimum, maximum) {
    return Math.max(minimum, Math.min(maximum, value));
}
export function roundCurrency(value) {
    return Math.round(value * 100) / 100;
}
export function percentage(value, rate) {
    return roundCurrency(value * clamp(rate, 0, 1));
}
