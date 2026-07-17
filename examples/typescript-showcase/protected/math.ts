// @venom: protected module
export function clamp(value: number, minimum: number, maximum: number): number {
  return Math.max(minimum, Math.min(maximum, value));
}

export function roundCurrency(value: number): number {
  return Math.round(value * 100) / 100;
}

export function percentage(value: number, rate: number): number {
  return roundCurrency(value * clamp(rate, 0, 1));
}
