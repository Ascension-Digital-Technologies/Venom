// @venom: browser
const pageMarker = 1;

// @venom: input value:number, bonus?:integer
// @venom: output result:number
// @venom: protected
export async function score(input) {
  return { result: input.value * 2 + (input.bonus || 0) };
}

const answer = await score({ value: 4, bonus: 1 });
document.body.dataset.result = String(answer.result);
