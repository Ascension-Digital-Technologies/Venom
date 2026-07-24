// @venom: browser
'use strict';

import { analyzeVelocity } from './protected/velocity-engine.js';

// Keep an explicit facade binding in the offscreen host so Venom exports the
// protected function into the runtime bridge. Calls still arrive through the
// generated extension RPC host and never execute this adapter directly.
globalThis.__VELOCITY_PROTECTED_ENGINE_HOST__ = Object.freeze({
  analyzeVelocity
});
