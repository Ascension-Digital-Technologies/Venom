  function normalizeModuleSpecifier(specifier, referrer = '') {
    const raw = String(specifier || '').trim();
    if (!raw) return '';
    if (/^[a-zA-Z][a-zA-Z0-9+.-]*:/.test(raw) || raw.startsWith('//')) return 'blocked:' + raw;
    if (!raw.startsWith('.') && !raw.startsWith('/')) return raw.replace(/^\/+/, '');
    const base = String(referrer || '').split('/');
    if (base.length) base.pop();
    for (const part of raw.split('/')) {
      if (!part || part === '.') continue;
      if (part === '..') base.pop();
      else base.push(part);
    }
    return base.filter(Boolean).join('/');
  }

  function recordResolverAudit(specifier, referrer, normalized, status) {
    const item = Object.freeze({ id: resolverAuditRecords.length + 1, specifier: String(specifier || ''), referrer: String(referrer || ''), normalized: String(normalized || ''), status: String(status || 'unknown'), hostCallId: 4 });
    resolverAuditRecords.push(item);
    return item;
  }

  function importModuleNamespace(specifier, referrer) {
    const normalized = normalizeModuleSpecifier(specifier, referrer);
    if (!normalized || normalized.startsWith('blocked:')) {
      recordResolverAudit(specifier, referrer, normalized, 'blocked');
      if (config.resolverAudit && String(config.resolverAudit.unknownSpecifier || '').includes('trap')) throw new Error('TurboJS module resolver blocked ' + specifier);
      return Object.freeze({});
    }
    const value = moduleNamespaceCache.get(normalized) || moduleNamespaceCache.get(String(specifier || '')) || null;
    recordResolverAudit(specifier, referrer, normalized, value ? 'resolved-cache' : 'missing-empty-namespace');
    return value || Object.freeze({});
  }


  function sourceDeclaresInjectedBinding(source, name) {
    const id = String(name || '');
    if (!/^[A-Za-z_$][\w$]*$/.test(id)) return false;
    const pattern = new RegExp('(^|[^A-Za-z0-9_$])(?:const|let|class|function)\\s+' + id + '\\b');
    return pattern.test(String(source || ''));
  }

  function exportBindingStatements(items) {
    const out = [];
    for (const item of items) {
      const local = item.local || item.name;
      const exported = item.exported || item.name;
      if (local && exported) out.push('__venomExport(' + JSON.stringify(exported) + ', ' + local + ');');
    }
    return out.join('\n');
  }

  function transformModuleSource(source, chunk) {
    let code = String(source || '');
    if (/\bimport\s*\(/.test(code)) throw new Error('dynamic import is trapped by TurboJS module execution prototype');
    if (/\bawait\b/.test(code) && /^\s*await\b/m.test(code)) throw new Error('top-level await is not enabled in TurboJS module execution prototype');
    const imports = [];
    const exports = [];
    code = code.replace(/import\s+\*\s+as\s+([A-Za-z_$][\w$]*)\s+from\s+['"]([^'"]+)['"]\s*;?/g, (_, ns, spec) => { imports.push(spec); return 'const ' + ns + ' = __venomImport(' + JSON.stringify(spec) + ');'; });
    code = code.replace(/import\s+\{([^}]+)\}\s+from\s+['"]([^'"]+)['"]\s*;?/g, (_, names, spec) => {
      imports.push(spec);
      const bindings = names.split(',').map((part) => {
        const bits = part.trim().split(/\s+as\s+/);
        return bits.length === 2 ? bits[0].trim() + ': ' + bits[1].trim() : bits[0].trim();
      }).filter(Boolean).join(', ');
      return 'const { ' + bindings + ' } = __venomImport(' + JSON.stringify(spec) + ');';
    });
    code = code.replace(/import\s+([A-Za-z_$][\w$]*)\s+from\s+['"]([^'"]+)['"]\s*;?/g, (_, name, spec) => { imports.push(spec); return 'const ' + name + ' = __venomImport(' + JSON.stringify(spec) + ').default;'; });
    code = code.replace(/import\s+['"]([^'"]+)['"]\s*;?/g, (_, spec) => { imports.push(spec); return '__venomImport(' + JSON.stringify(spec) + ');'; });
    code = code.replace(/export\s+function\s+([A-Za-z_$][\w$]*)\s*\(/g, (_, name) => { exports.push({ name }); return 'function ' + name + '('; });
    code = code.replace(/export\s+(const|let|var)\s+([A-Za-z_$][\w$]*)\s*=/g, (_, kind, name) => { exports.push({ name }); return kind + ' ' + name + ' ='; });
    code = code.replace(/export\s+default\s+([^;]+);?/g, (_, expr) => { exports.push({ local: '__venomDefaultExport', exported: 'default' }); return 'const __venomDefaultExport = ' + expr + ';'; });
    code = code.replace(/export\s+\{([^}]+)\}\s*;?/g, (_, names) => {
      const items = names.split(',').map((part) => {
        const bits = part.trim().split(/\s+as\s+/);
        return bits.length === 2 ? { local: bits[0].trim(), exported: bits[1].trim() } : { local: bits[0].trim(), exported: bits[0].trim() };
      }).filter((item) => item.local);
      exports.push(...items);
      return exportBindingStatements(items);
    });
    const tail = exportBindingStatements(exports.filter((item) => item.name));
    return Object.freeze({ code: tail ? code + '\n' + tail : code, imports: Object.freeze(imports.slice()), exports: Object.freeze(exports.map((item) => item.exported || item.name || 'default')) });
  }

  async function executeChunk(input = {}) {
    let chunk = input.chunk || {};
    validateBytecodeTrustHandoff(chunk);
    const context = createContext(input.context || {});
    const bindings = buildExecutionBindings(input);
    if ((!chunk.code || !String(chunk.code).trim()) && !(chunk.bytecodeBytes && chunk.bytecodeBytes.length)) {
      const empty = Object.freeze({ executed: false, empty: true, engineModule: true, moduleVersion, mode, context: context.key, turboJsWasm: !!wasmUrl });
      boundedPush(executions, empty, maxExecutionRecords);
      return empty;
    }
    const fallbackText = String(fallback || '');
    const policyAllowsFunction = !!(config.policy && config.policy.allowFunctionConstructor);
    const policyAllowsEval = !!(config.policy && config.policy.allowEval);
    const fallbackPolicyStrict = !!(config.fallbackPolicy && (config.fallbackPolicy.enabled === false || config.fallbackPolicy.strictRelease || String(config.fallbackPolicy.currentReleasePolicy || '').startsWith('deny-host-fallback')));
    const fallbackDenied = fallbackPolicyStrict || ((!policyAllowsFunction && !policyAllowsEval) && (fallbackText.includes('deny-host-js-source-eval') || fallbackText.includes('fail-closed') || fallbackText.includes('deny-host-js')));
    const wasmResult = await enqueueWasmExecution(() => executeWithWasmScaffold(chunk, context));
    if (wasmResult) {
      boundedPush(resultRecords, Object.freeze({ id: resultRecords.length + 1, route: chunk.route || '', source: chunk.source || '', order: chunk.order >>> 0, wasmReport: wasmResult.report, executionRecord: wasmResult.executionRecord, fallbackRequired: wasmResult.fallbackRequired, hostBridgeTelemetry: wasmResult.hostBridgeTelemetry, hostBridgeParity: wasmResult.hostBridgeParity }), maxExecutionRecords);
      // v0.37 records the WASM backend result and falls back only through the explicit policy gate.
      if (bindings.console && typeof bindings.console.debug === 'function') {
        bindings.console.debug('[venom] TurboJS WASM backend accepted bytecode chunk', chunk.source || 'inline');
      }
      if (fallbackDenied && wasmResult) {
        const bridgeReplay = replayWasmHostBridgeEffects(chunk, wasmResult.hostBridgeTelemetry, bindings);
        const nativeHostEffectCount = Array.isArray(wasmResult.hostEffects) ? wasmResult.hostEffects.length : 0;
        const hostBridgeParity = Object.freeze({ ...(wasmResult.hostBridgeTelemetry || {}), effectReplay: bridgeReplay, effectReplayCount: 0, nativeHostEffectCount, actualHostEffects: nativeHostEffectCount > 0, replayMode: bridgeReplay.mode, replayEvalUsed: false, replayFunctionConstructorUsed: false });
        const effectsApplied = applyWasmHostEffects(wasmResult.hostEffects, bindings);
        const backendAccepted = !!(wasmResult.executed || wasmResult.report || wasmResult.executionRecord);
        const result = Object.freeze({ executed: backendAccepted, engineModule: true, moduleVersion, mode, context: context.key, fallback: 'denied-host-js-source-eval', turboJsWasm: true, bytecodeExecuted: !!wasmResult.bytecodeExecuted, wasmReport: wasmResult.report, wasmExecutionRecord: wasmResult.executionRecord, fallbackRequired: false, hostEffectsApplied: effectsApplied, hostBridgeEffectReplay: bridgeReplay, module: false, consoleEvents: consoleEvents.length, resultRecordCount: resultRecords.length, capabilities: bindings.__venomCapabilities ? bindings.__venomCapabilities.length : 0, sourceEvalUsed: false, hostBridgeTelemetry: hostBridgeParity, hostBridgeParity });
        boundedPush(executions, result, maxExecutionRecords);
        return result;
      }
      if (wasmResult.fallbackRequired && fallbackDenied) throw new Error('TurboJS fallback policy denied backend fallback');
    }
    if (!chunk.code && chunk.bytecodeBytes && chunk.bytecodeBytes.length) {
      if (fallbackDenied) throw new Error('TurboJS WASM interpreter unavailable; source-decode fallback denied: ' + wasmFailureDetail());
      chunk = { ...chunk, code: decodeTurboJsRecordForDeniedFallback(chunk.bytecodeBytes, chunk.source) };
    }
    if (fallbackDenied) throw new Error('TurboJS/WASM backend failed; protected host source execution is denied');
    const isModule = ((chunk.flags >>> 0) & SCRIPT_FLAG_MODULE) !== 0 || /(^|\n)\s*(import|export)\s+/m.test(String(chunk.code || ''));
    const moduleNamespace = Object.create(null);
    const moduleImportRecords = [];
    const moduleExportRecords = [];
    const __venomImport = (specifier) => {
      const namespace = importModuleNamespace(specifier, chunk.source || context.source || '');
      moduleImportRecords.push(Object.freeze({ specifier: String(specifier || ''), resolved: normalizeModuleSpecifier(specifier, chunk.source || context.source || '') }));
      return namespace;
    };
    const __venomExport = (name, value) => {
      moduleNamespace[String(name || 'default')] = value;
      moduleExportRecords.push(String(name || 'default'));
      return value;
    };
    const transformed = isModule ? transformModuleSource(chunk.code, chunk) : Object.freeze({ code: String(chunk.code || ''), imports: Object.freeze([]), exports: Object.freeze([]) });
    const evalBindings = isModule ? Object.freeze({ ...bindings, __venomImport, __venomExport }) : bindings;
__VENOM_TURBOJS_FALLBACK_BLOCK__
    const g = bindings.globalThis || globalThis;
    g.__venomTurboJsModuleProbe = (g.__venomTurboJsModuleProbe || 0) + 1;
    let frozenNamespace = null;
    if (isModule) {
      frozenNamespace = Object.freeze({ ...moduleNamespace });
      const cacheKey = normalizeModuleSpecifier(chunk.source || String(chunk.order >>> 0), chunk.source || context.source || '');
      moduleNamespaceCache.set(cacheKey, frozenNamespace);
      if (chunk.source) moduleNamespaceCache.set(String(chunk.source), frozenNamespace);
      boundedPush(moduleExecutions, Object.freeze({ id: moduleExecutions.length + 1, source: String(chunk.source || ''), route: String(chunk.route || context.route || '/'), imports: Object.freeze(moduleImportRecords.slice()), exports: Object.freeze(moduleExportRecords.slice()), cacheKey }), maxExecutionRecords);
    }
    const result = Object.freeze({
      executed: true,
      engineModule: true,
      moduleVersion,
      mode,
      context: context.key,
      fallback,
      turboJsWasm: !!wasmResult,
      wasmReport: wasmResult ? wasmResult.report : null,
      wasmExecutionRecord: wasmResult ? wasmResult.executionRecord : null,
      hostBridgeTelemetry: wasmResult ? wasmResult.hostBridgeTelemetry : null,
      hostBridgeParity: wasmResult ? wasmResult.hostBridgeParity : null,
      fallbackRequired: wasmResult ? wasmResult.fallbackRequired : false,
      module: isModule,
      moduleImports: moduleImportRecords.length,
      moduleExports: moduleExportRecords.length,
      moduleCacheSize: moduleNamespaceCache.size,
      resolverAuditCount: resolverAuditRecords.length,
      consoleEvents: consoleEvents.length,
      resultRecordCount: resultRecords.length,
      capabilities: bindings.__venomCapabilities ? bindings.__venomCapabilities.length : 0,
    });
    boundedPush(executions, result, maxExecutionRecords);
    return result;
  }

