// @venom: browser
export function money(value:number):string { return new Intl.NumberFormat("en-US", { style:"currency", currency:"USD", maximumFractionDigits:0 }).format(value); }
export function compact(value:number):string { return new Intl.NumberFormat("en-US", { notation:"compact", maximumFractionDigits:1 }).format(value); }
export function percent(value:number):string { return `${Math.round(value)}%`; }
export function age(minutes:number):string { if (minutes < 60) return `${minutes}m`; const hours=Math.floor(minutes/60); return `${hours}h ${minutes%60}m`; }
export function titleCase(value:string):string { return value.replace(/\b\w/g, letter => letter.toUpperCase()); }
