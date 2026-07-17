import { isOdd } from "./cycle-b";
export function isEven(value: number): boolean { return value === 0 || isOdd(value - 1); }
