// @venom: protected module
const multiplier = 3;
function internal(value) { return value * multiplier; }
export function score(input) { return internal(input.value); }
