import { leafValue } from './module-leaf.js';
export const branchValue = leafValue + 6;
globalThis.__venomCompatModuleOrder = (globalThis.__venomCompatModuleOrder || []).concat('branch');
globalThis.__venomCompatModuleBranch = branchValue;
