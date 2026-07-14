// @venom: browser

// @venom: protected
export async function scoreNamed(
  { value, bonus = (2 + 3) },
  [scale = 4] = [4],
  options = { clamp: { min: 0, max: 100 } }
) {
  const raw = (value + bonus) * scale;
  return Math.max(options.clamp.min, Math.min(options.clamp.max, raw));
}

const result = await scoreNamed({ value: 3 }, [5]);
console.log(result);
