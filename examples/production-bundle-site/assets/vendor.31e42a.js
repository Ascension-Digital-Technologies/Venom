export const clamp=(e,t,n)=>Math.min(n,Math.max(t,e));
export const sum=e=>e.reduce((e,t)=>e+t,0);
export const label=e=>`item-${String(e).padStart(2,"0")}`;
