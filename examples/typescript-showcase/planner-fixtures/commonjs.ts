declare const require: (name: string) => unknown;
export function loadLegacy(name: string): unknown { return require(name); }
