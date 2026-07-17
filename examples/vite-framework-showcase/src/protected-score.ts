// @venom: protected
export async function calculateProtectedScore(values: number[]): Promise<number> {
  return values.reduce((total, value, index) => total + value * (index + 1), 0);
}
