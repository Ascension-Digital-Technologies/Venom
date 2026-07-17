import { isEven } from "./cycle-a";
export function isOdd(value: number): boolean { return value !== 0 && isEven(value - 1); }
