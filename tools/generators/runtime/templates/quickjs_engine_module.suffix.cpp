)QJSENGINE";
  {
    const std::string required_begin = "  const RELEASE_ABI_REQUIRED_EXPORTS = Object.freeze([";
    const std::string required_end = "  ]);";
    const auto begin = js.find(required_begin);
    const auto end = begin == std::string::npos ? std::string::npos : js.find(required_end, begin);
    if (begin == std::string::npos || end == std::string::npos) throw std::runtime_error("QuickJS ABI export marker missing");
    std::ostringstream generated;
    generated << required_begin << "\n";
    for (const auto name : generated::quickjs_wasm_abi::required_exports) generated << "    '" << name << "',\n";
    generated << required_end;
    js.replace(begin, end + required_end.size() - begin, generated.str());

    const std::string approved_begin = "  const RELEASE_ABI_APPROVED_TOOLCHAIN_EXPORTS = new Map([";
    const auto approved_pos = js.find(approved_begin);
    const auto approved_end = approved_pos == std::string::npos ? std::string::npos : js.find("  ]);", approved_pos);
    if (approved_pos == std::string::npos || approved_end == std::string::npos) throw std::runtime_error("QuickJS toolchain ABI marker missing");
    std::ostringstream approved;
    approved << approved_begin << "\n";
    for (const auto name : generated::quickjs_wasm_abi::allowed_toolchain_exports) {
      const auto kind = name == "__indirect_function_table" ? "table" : "function";
      approved << "    ['" << name << "', '" << kind << "'],\n";
    }
    approved << "  ]);";
    js.replace(approved_pos, approved_end + 5u - approved_pos, approved.str());
  }

  const std::string fallback_marker = "__VENOM_QUICKJS_FALLBACK_BLOCK__";
  const auto fallback_pos = js.find(fallback_marker);
  if (fallback_pos == std::string::npos) throw std::runtime_error("QuickJS module generation marker missing");
  const std::string fallback_block = protected_release
      ? "    throw new Error('QuickJS/WASM execution failed; protected host fallback is unavailable');\n"
      : "    const names = Object.keys(evalBindings).filter((name) => !sourceDeclaresInjectedBinding(transformed.code, name));\n"
        "    const values = names.map((name) => evalBindings[name]);\n"
        "    const wrapped = '\"use strict\";\\n' + transformed.code + '\\n//# sourceURL=venom-qjs-module://' + safeSourceUrl(chunk.source);\n"
        "    const fn = new Function(...names, wrapped);\n"
        "    fn(...values);\n";
  js.replace(fallback_pos, fallback_marker.size(), fallback_block);
  if (protected_release) {
    const std::string release_flag = "  const protectedRelease = false;";
    const auto release_flag_pos = js.find(release_flag);
    if (release_flag_pos == std::string::npos) throw std::runtime_error("protected QuickJS module release flag marker missing");
    js.replace(release_flag_pos, release_flag.size(), "  const protectedRelease = true;");

    const std::string full_surface = R"SURFACE(  return Object.freeze({
    kind: 'venom.quickjs.engine.module',
    version: moduleVersion,
    mode,
    fallback,
    wasmUrl,
    createContext,
    destroyContext,
    contextSnapshot,
    status,
    executionSnapshot() { return Object.freeze({ count: executions.length, executions: Object.freeze(executions.slice()), resultRecords: Object.freeze(resultRecords.slice()) }); },
    consoleEvents() { return Object.freeze(consoleEvents.slice()); },
    clearConsoleEvents() { const count = consoleEvents.length; consoleEvents.length = 0; return count; },
    fallbackPolicy() { return Object.freeze({ mode: fallback, configured: !!config.fallbackPolicy, strictRelease: !!(config.fallbackPolicy && config.fallbackPolicy.strictRelease) }); },
    abiTable,
    hostImportTable,
    parityProbe,
    bytecodeManifest,
    moduleResolver,
    exceptionAbi,
    hostTrapPolicy,
    executionLifecycle,
    scriptBufferPolicy,
    contextLimitPolicy,
    hostCallDispatch,
    parityContract,
    releaseFailClosed,
    moduleGraph,
    moduleCacheSnapshot,
    resolverAudit,
    interopFallback,
    executionJournal,
    checkpointPolicy,
    replayCursor,
    resumeState,
    determinismAudit,
    snapshotPolicy() { return cachedSnapshotPolicy || Object.freeze({ loaded: false }); },
    snapshotRecord() { return cachedSnapshotRecord || Object.freeze({ loaded: false }); },
    replayValidation() { return cachedReplayValidation || Object.freeze({ loaded: false }); },
    determinismLedger() { return cachedDeterminismLedger || Object.freeze({ loaded: false }); },
    auditSeal() { return cachedAuditSeal || Object.freeze({ loaded: false }); },
    executionCommit() { return cachedExecutionCommit || Object.freeze({ loaded: false }); },
    rollbackPolicy() { return cachedRollbackPolicy || Object.freeze({ loaded: false }); },
    hostCallReceipts() { return cachedHostReceipts || Object.freeze({ loaded: false }); },
    releaseAcceptance() { return cachedReleaseAcceptance || Object.freeze({ loaded: false }); },
    commitAudit() { return cachedCommitAudit || Object.freeze({ loaded: false }); },
    capabilityPolicy() { return cachedCapabilityPolicy || Object.freeze({ loaded: false }); },
    hostIoPolicy() { return cachedHostIoPolicy || Object.freeze({ loaded: false }); },
    permissionSeal() { return cachedPermissionSeal || Object.freeze({ loaded: false }); },
    policyReceipts() { return cachedPolicyReceipts || Object.freeze({ loaded: false }); },
    releaseGate() { return cachedReleaseGate || Object.freeze({ loaded: false }); },
    hostIoDecision() { return cachedHostIoDecision || Object.freeze({ loaded: false }); },
    hostIoDenyTrace() { return cachedHostIoDenyTrace || Object.freeze({ loaded: false }); },
    capabilityLedger() { return cachedCapabilityLedger || Object.freeze({ loaded: false }); },
    policySealAudit() { return cachedPolicySealAudit || Object.freeze({ loaded: false }); },
    runtimeDenylist() { return cachedRuntimeDenylist || Object.freeze({ loaded: false }); },
    executeChunk,
  });
)SURFACE";
    const auto surface_pos = js.rfind(full_surface);
    if (surface_pos == std::string::npos) throw std::runtime_error("protected QuickJS module surface marker missing");
    const std::string minimal_surface = R"SURFACE(  return Object.freeze({
    kind: 'venom.quickjs.engine.module', version: moduleVersion, mode, wasmUrl,
    createContext, destroyContext, contextSnapshot, status,
    executionSnapshot() { return Object.freeze({ count: executions.length }); },
    consoleEvents() { return Object.freeze(consoleEvents.slice()); },
    clearConsoleEvents() { const count = consoleEvents.length; consoleEvents.length = 0; return count; },
    executeChunk,
  });
)SURFACE";
    js.replace(surface_pos, full_surface.size(), minimal_surface);
  }
  return js;
}


} // namespace venom::compiler
