# Dynamic module and CommonJS boundaries

Venom's AST planner records module-loading behavior that cannot be lowered into a statically closed protected dependency graph.

The following forms require manual review by default:

```js
await import('./feature.js');
await import('./' + name + '.js');
const feature = require('./feature.js');
module.exports = feature;
exports.run = feature.run;
```

Literal dynamic imports are resolved for graph visibility, but remain a runtime execution boundary because protected TurboJS module lowering does not currently preserve browser bundler chunk loading semantics. Non-literal imports are additionally unresolved at build time.

CommonJS is treated conservatively because `require`, mutable export objects, loader caches, and cyclic initialization have runtime identity semantics that differ from native ES modules.

Explicit file annotations and planner rules remain authoritative when the developer intentionally assigns these modules to browser execution.
