// @venom: protected module
import { score as privateScore } from "./core.js";
function internal(value) { return privateScore({ value }) + 1; }
// @venom: input value:number
// @venom: output result:number
export function calculate(input) { return { result: internal(input.value) }; }
// @venom: input value?:number
// @venom: output result:number
export async function calculateAsync(input = { value: 0 }) { return { result: internal(input.value) + 1 }; }
