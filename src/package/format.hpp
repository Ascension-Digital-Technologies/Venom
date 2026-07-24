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
  PackageFlagTurboJsBridge = 1u << 15u,
  PackageFlagScriptIsolation = 1u << 16u,
  PackageFlagScriptPolicy = 1u << 17u,
  PackageFlagTurboJsChunks = 1u << 18u,
  PackageFlagTurboJsEngine = 1u << 19u,
  PackageFlagScriptEngineFallback = 1u << 20u,
  PackageFlagTurboJsEngineModule = 1u << 21u,
  PackageFlagTurboJsContextLifecycle = 1u << 22u,
  PackageFlagHostCapabilities = 1u << 23u,
  PackageFlagTurboJsAdapterDiagnostics = 1u << 24u,
  PackageFlagTurboJsWasmRuntime = 1u << 25u,
  PackageFlagTurboJsSourceTransfer = 1u << 26u,
  PackageFlagTurboJsConsoleBridge = 1u << 27u,
  PackageFlagTurboJsExecutionRecords = 1u << 28u,
  PackageFlagTurboJsResultBridge = 1u << 29u,
  PackageFlagTurboJsFallbackPolicy = 1u << 30u,
  PackageFlagTurboJsEngineBackend = 1u << 31u,
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
  TurboJs = 6,
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
  TurboJsBridge = 17,
  ScriptIsolation = 18,
  ScriptPolicy = 19,
  TurboJsChunks = 20,
  TurboJsEngine = 21,
  ScriptEnginePolicy = 22,
  TurboJsEngineModule = 23,
  TurboJsContextLifecycle = 24,
  HostCapabilities = 25,
  TurboJsAdapterDiagnostics = 26,
  TurboJsWasmRuntime = 27,
  TurboJsSourceTransfer = 28,
  TurboJsConsoleBridge = 29,
  TurboJsExecutionRecords = 30,
  TurboJsResultBridge = 31,
  TurboJsFallbackPolicy = 32,
  TurboJsEngineBackend = 33,
  TurboJsNativeParity = 34,
  TurboJsExecutionMode = 35,
  TurboJsRuntimeAbi = 36,
  TurboJsHostImports = 37,
  TurboJsHeapLimits = 38,
  TurboJsScriptBuffer = 39,
  TurboJsConsoleAbi = 40,
  TurboJsParityProbe = 41,
  TurboJsReleaseFallback = 42,
  TurboJsBytecodeManifest = 43,
  TurboJsModuleResolver = 44,
  TurboJsExceptionAbi = 45,
  TurboJsHostTrapPolicy = 46,
  TurboJsExecutionLifecycle = 47,
  TurboJsScriptBufferPolicy = 48,
  TurboJsContextLimitPolicy = 49,
  TurboJsHostCallDispatch = 50,
  TurboJsParityContract = 51,
  TurboJsReleaseFailClosed = 52,
  TurboJsModuleGraph = 53,
  TurboJsModuleExecution = 54,
  TurboJsModuleCache = 55,
  TurboJsResolverAudit = 56,
  TurboJsInteropFallback = 57,
  TurboJsExecutionPipeline = 58,
  TurboJsScriptRecords = 59,
  TurboJsEvalResults = 60,
  TurboJsConsoleCapture = 61,
  TurboJsFailureReports = 62,
  TurboJsExecutionJournal = 63,
  TurboJsCheckpointPolicy = 64,
  TurboJsReplayCursor = 65,
  TurboJsResumeState = 66,
  TurboJsDeterminismAudit = 67,
  TurboJsSnapshotPolicy = 68,
  TurboJsSnapshotRecords = 69,
  TurboJsReplayValidation = 70,
  TurboJsDeterminismLedger = 71,
  TurboJsAuditSeal = 72,
  TurboJsExecutionCommit = 73,
  TurboJsRollbackPolicy = 74,
  TurboJsHostCallReceipts = 75,
  TurboJsReleaseAcceptance = 76,
  TurboJsCommitAudit = 77,
  TurboJsCapabilityPolicy = 78,
  TurboJsHostIoPolicy = 79,
  TurboJsPermissionSeal = 80,
  TurboJsPolicyReceipts = 81,
  TurboJsReleaseGate = 82,
  TurboJsHostIoDecision = 83,
  TurboJsHostIoDenyTrace = 84,
  TurboJsCapabilityLedger = 85,
  TurboJsPolicySealAudit = 86,
  TurboJsRuntimeDenylist = 87,
  PackageFeatures = 88,
};

const char* section_type_name(SectionType type);
bool is_known_section_type(std::uint32_t value);
bool should_redact_section_name(SectionType type);
std::string protected_section_alias(SectionType type, const std::string& canonical_name);
bool section_name_matches(SectionType type, const std::string& stored_name, const std::string& canonical_name);
void validate_section_name(const std::string& name);

} // namespace venom::package
