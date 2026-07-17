#pragma once

#include <cstdint>
#include <string>
#include "generated/contracts/product_contracts.hpp"

namespace venom::package {

constexpr char kMagic[8] = {'V','E','N','O','M','V','B','C'};
constexpr std::uint32_t kVersion = venom::contracts::kPackageFormatVersion;
constexpr std::uint32_t kRuntimeAbi = venom::contracts::kPackageRuntimeAbi;
constexpr std::uint64_t kHeaderSize = 80;
constexpr std::uint64_t kSectionEntrySize = 48;
constexpr std::uint64_t kPackageHashOffset = 72;

// Package flags are profile-level metadata. They are intentionally not security
// claims by themselves; protection/encryption providers are connected later.
enum PackageFlags : std::uint32_t {
  PackageFlagNone = 0u,
  PackageFlagProtectProfile = 1u << 0u,
  PackageFlagReleaseProfile = 1u << 1u,
  PackageFlagPolymorphic = 1u << 2u,
  PackageFlagCompressedSections = 1u << 3u,
  PackageFlagCryptoProviderReady = 1u << 4u,
  PackageFlagIntegrityMetadata = 1u << 5u,
  PackageFlagAeadProviderReady = 1u << 6u,
  PackageFlagRuntimeHardened = 1u << 7u,
  PackageFlagWasmRuntime = 1u << 8u,
  PackageFlagHostBridge = 1u << 9u,
  PackageFlagBinaryDomOps = 1u << 10u,
  PackageFlagFetchBridge = 1u << 11u,
  PackageFlagAsyncHostQueue = 1u << 12u,
  PackageFlagTimerBridge = 1u << 13u,
  PackageFlagEventQueue = 1u << 14u,
  PackageFlagQuickJsBridge = 1u << 15u,
  PackageFlagScriptIsolation = 1u << 16u,
  PackageFlagScriptPolicy = 1u << 17u,
  PackageFlagQuickJsChunks = 1u << 18u,
  PackageFlagQuickJsEngine = 1u << 19u,
  PackageFlagScriptEngineFallback = 1u << 20u,
  PackageFlagQuickJsEngineModule = 1u << 21u,
  PackageFlagQuickJsContextLifecycle = 1u << 22u,
  PackageFlagHostCapabilities = 1u << 23u,
  PackageFlagQuickJsAdapterDiagnostics = 1u << 24u,
  PackageFlagQuickJsWasmRuntime = 1u << 25u,
  PackageFlagQuickJsSourceTransfer = 1u << 26u,
  PackageFlagQuickJsConsoleBridge = 1u << 27u,
  PackageFlagQuickJsExecutionRecords = 1u << 28u,
  PackageFlagQuickJsResultBridge = 1u << 29u,
  PackageFlagQuickJsFallbackPolicy = 1u << 30u,
  PackageFlagQuickJsEngineBackend = 1u << 31u,
};

enum SectionFlags : std::uint32_t {
  SectionFlagNone = 0u,
  SectionFlagCompressed = 1u << 0u,
  SectionFlagEncrypted = 1u << 1u,
};

enum class SectionType : std::uint32_t {
  Manifest = 1,
  Routes = 2,
  DomTemplates = 3,
  Css = 4,
  JavaScript = 5,
  QuickJs = 6,
  VmBytecode = 7,
  Asset = 8,
  Integrity = 9,
  Strings = 10,
  AssetManifest = 11,
  HostBridge = 12,
  FetchBridge = 13,
  AsyncHostQueue = 14,
  TimerBridge = 15,
  EventQueue = 16,
  QuickJsBridge = 17,
  ScriptIsolation = 18,
  ScriptPolicy = 19,
  QuickJsChunks = 20,
  QuickJsEngine = 21,
  ScriptEnginePolicy = 22,
  QuickJsEngineModule = 23,
  QuickJsContextLifecycle = 24,
  HostCapabilities = 25,
  QuickJsAdapterDiagnostics = 26,
  QuickJsWasmRuntime = 27,
  QuickJsSourceTransfer = 28,
  QuickJsConsoleBridge = 29,
  QuickJsExecutionRecords = 30,
  QuickJsResultBridge = 31,
  QuickJsFallbackPolicy = 32,
  QuickJsEngineBackend = 33,
  QuickJsNativeParity = 34,
  QuickJsExecutionMode = 35,
  QuickJsRuntimeAbi = 36,
  QuickJsHostImports = 37,
  QuickJsHeapLimits = 38,
  QuickJsScriptBuffer = 39,
  QuickJsConsoleAbi = 40,
  QuickJsParityProbe = 41,
  QuickJsReleaseFallback = 42,
  QuickJsBytecodeManifest = 43,
  QuickJsModuleResolver = 44,
  QuickJsExceptionAbi = 45,
  QuickJsHostTrapPolicy = 46,
  QuickJsExecutionLifecycle = 47,
  QuickJsScriptBufferPolicy = 48,
  QuickJsContextLimitPolicy = 49,
  QuickJsHostCallDispatch = 50,
  QuickJsParityContract = 51,
  QuickJsReleaseFailClosed = 52,
  QuickJsModuleGraph = 53,
  QuickJsModuleExecution = 54,
  QuickJsModuleCache = 55,
  QuickJsResolverAudit = 56,
  QuickJsInteropFallback = 57,
  QuickJsExecutionPipeline = 58,
  QuickJsScriptRecords = 59,
  QuickJsEvalResults = 60,
  QuickJsConsoleCapture = 61,
  QuickJsFailureReports = 62,
  QuickJsExecutionJournal = 63,
  QuickJsCheckpointPolicy = 64,
  QuickJsReplayCursor = 65,
  QuickJsResumeState = 66,
  QuickJsDeterminismAudit = 67,
  QuickJsSnapshotPolicy = 68,
  QuickJsSnapshotRecords = 69,
  QuickJsReplayValidation = 70,
  QuickJsDeterminismLedger = 71,
  QuickJsAuditSeal = 72,
  QuickJsExecutionCommit = 73,
  QuickJsRollbackPolicy = 74,
  QuickJsHostCallReceipts = 75,
  QuickJsReleaseAcceptance = 76,
  QuickJsCommitAudit = 77,
  QuickJsCapabilityPolicy = 78,
  QuickJsHostIoPolicy = 79,
  QuickJsPermissionSeal = 80,
  QuickJsPolicyReceipts = 81,
  QuickJsReleaseGate = 82,
  QuickJsHostIoDecision = 83,
  QuickJsHostIoDenyTrace = 84,
  QuickJsCapabilityLedger = 85,
  QuickJsPolicySealAudit = 86,
  QuickJsRuntimeDenylist = 87,
  PackageFeatures = 88,
};

const char* section_type_name(SectionType type);
bool is_known_section_type(std::uint32_t value);
bool should_redact_section_name(SectionType type);
std::string protected_section_alias(SectionType type, const std::string& canonical_name);
bool section_name_matches(SectionType type, const std::string& stored_name, const std::string& canonical_name);

} // namespace venom::package
