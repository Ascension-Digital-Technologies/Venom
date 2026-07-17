export async function loadFeature(name: string): Promise<unknown> { return import("./" + name); }
