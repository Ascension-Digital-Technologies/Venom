export async function loadFeature(name:string){ return import(`./${name}`); }
