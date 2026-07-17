import { cycleB } from "./cycle-b"; export function cycleA(value:number):number { return value<=0?0:cycleB(value-1); }
