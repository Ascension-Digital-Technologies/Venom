// @venom: browser

// @venom: protected
export const protectedScore = async (
  value,
  bonus
) => {
  const marker = "A12_EXPORTED_MULTILINE_ARROW";
  return value * 7 + bonus + marker.length;
};

const result = await protectedScore(3, 2);
document.body.dataset.result = String(result);
