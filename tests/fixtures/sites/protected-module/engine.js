// @venom: protected module
// import { fake } from "./fake-comment.js";
const fakeImportText = "import { fake } from './fake-string.js'";
import {
  score as privateScore
} from "./core.js";
function internal(value) { return privateScore({ value }) + 1; }
// @venom: input value:number
// @venom: output result:number
export function calculate({ value: inputValue }) { return { result: internal(inputValue) }; }
// @venom: input value?:number
// @venom: output result:number
export async function calculateAsync(input = { value: 0 }) { return { result: internal(input.value) + 1 }; }
