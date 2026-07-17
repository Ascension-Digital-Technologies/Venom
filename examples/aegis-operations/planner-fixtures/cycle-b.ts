import { cycleA } from "./cycle-a"; export function cycleB(value:number):number { return value<=0?0:cycleA(value-1); }
