// @venom: protected
export function scoreRisk(values: number[]): number { return values.reduce((a,b)=>a+b,0); }
