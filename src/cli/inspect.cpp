#include "cli/inspect.hpp"

#include "package/format.hpp"
#include "package/reader.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace venom::compiler {

namespace {
std::string hex64(std::uint64_t value) {
  std::ostringstream out;
  out << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
  return out.str();
}

void print_flags(std::uint32_t flags) {
  if (flags == 0) {
    std::cout << "none";
    return;
  }
  bool first = true;
  const auto emit = [&](const char* name) {
    if (!first) std::cout << "|";
    std::cout << name;
    first = false;
  };
  if ((flags & venom::package::PackageFlagProtectProfile) != 0u) emit("protect");
  if ((flags & venom::package::PackageFlagReleaseProfile) != 0u) emit("release");
  if ((flags & venom::package::PackageFlagPolymorphic) != 0u) emit("polymorphic");
  if ((flags & venom::package::PackageFlagCompressedSections) != 0u) emit("compressed");
  if ((flags & venom::package::PackageFlagCryptoProviderReady) != 0u) emit("crypto-ready");
  if ((flags & venom::package::PackageFlagIntegrityMetadata) != 0u) emit("integrity-metadata");
  if ((flags & venom::package::PackageFlagAeadProviderReady) != 0u) emit("aead-ready");
  if ((flags & venom::package::PackageFlagRuntimeHardened) != 0u) emit("runtime-hardened");
  if ((flags & venom::package::PackageFlagWasmRuntime) != 0u) emit("wasm-runtime");
  if ((flags & venom::package::PackageFlagHostBridge) != 0u) emit("host-bridge");
  if ((flags & venom::package::PackageFlagBinaryDomOps) != 0u) emit("binary-dom-ops");
  if ((flags & venom::package::PackageFlagFetchBridge) != 0u) emit("fetch-bridge");
  if ((flags & venom::package::PackageFlagAsyncHostQueue) != 0u) emit("async-host-queue");
  if ((flags & venom::package::PackageFlagTimerBridge) != 0u) emit("timer-bridge");
  if ((flags & venom::package::PackageFlagEventQueue) != 0u) emit("event-queue");
  if ((flags & venom::package::PackageFlagTurboJsBridge) != 0u) emit("turbojs-bridge");
  if ((flags & venom::package::PackageFlagScriptIsolation) != 0u) emit("script-isolation");
  if ((flags & venom::package::PackageFlagScriptPolicy) != 0u) emit("script-policy");
  if ((flags & venom::package::PackageFlagTurboJsChunks) != 0u) emit("turbojs-chunks");
  if ((flags & venom::package::PackageFlagTurboJsEngine) != 0u) emit("turbojs-engine");
  if ((flags & venom::package::PackageFlagScriptEngineFallback) != 0u) emit("script-engine-fallback");
  if ((flags & venom::package::PackageFlagTurboJsEngineModule) != 0u) emit("turbojs-engine-module");
  if ((flags & venom::package::PackageFlagTurboJsContextLifecycle) != 0u) emit("turbojs-context-lifecycle");
  if ((flags & venom::package::PackageFlagHostCapabilities) != 0u) emit("host-capabilities");
  if ((flags & venom::package::PackageFlagTurboJsAdapterDiagnostics) != 0u) emit("turbojs-adapter-diagnostics");
  if ((flags & venom::package::PackageFlagTurboJsWasmRuntime) != 0u) emit("turbojs-wasm-runtime");
  if ((flags & venom::package::PackageFlagTurboJsSourceTransfer) != 0u) emit("turbojs-source-transfer");
  if ((flags & venom::package::PackageFlagTurboJsConsoleBridge) != 0u) emit("turbojs-console-bridge");
  if ((flags & venom::package::PackageFlagTurboJsExecutionRecords) != 0u) emit("turbojs-execution-records");
  if ((flags & venom::package::PackageFlagTurboJsResultBridge) != 0u) emit("turbojs-result-bridge");
  if ((flags & venom::package::PackageFlagTurboJsFallbackPolicy) != 0u) emit("turbojs-fallback-policy");
  if ((flags & venom::package::PackageFlagTurboJsEngineBackend) != 0u) emit("turbojs-engine-backend");
  const auto known = venom::package::PackageFlagProtectProfile |
                     venom::package::PackageFlagReleaseProfile |
                     venom::package::PackageFlagPolymorphic |
                     venom::package::PackageFlagCompressedSections |
                     venom::package::PackageFlagCryptoProviderReady |
                     venom::package::PackageFlagIntegrityMetadata |
                     venom::package::PackageFlagAeadProviderReady |
                     venom::package::PackageFlagRuntimeHardened |
                     venom::package::PackageFlagWasmRuntime |
                     venom::package::PackageFlagHostBridge |
                     venom::package::PackageFlagBinaryDomOps |
                     venom::package::PackageFlagFetchBridge |
                     venom::package::PackageFlagAsyncHostQueue |
                     venom::package::PackageFlagTimerBridge |
                     venom::package::PackageFlagEventQueue |
                     venom::package::PackageFlagTurboJsBridge |
                     venom::package::PackageFlagScriptIsolation |
                     venom::package::PackageFlagScriptPolicy |
                     venom::package::PackageFlagTurboJsChunks |
                     venom::package::PackageFlagTurboJsEngine |
                     venom::package::PackageFlagScriptEngineFallback |
                     venom::package::PackageFlagTurboJsEngineModule |
                     venom::package::PackageFlagTurboJsContextLifecycle |
                     venom::package::PackageFlagHostCapabilities |
                     venom::package::PackageFlagTurboJsAdapterDiagnostics |
                     venom::package::PackageFlagTurboJsWasmRuntime |
                     venom::package::PackageFlagTurboJsSourceTransfer |
                     venom::package::PackageFlagTurboJsConsoleBridge |
                     venom::package::PackageFlagTurboJsExecutionRecords |
                     venom::package::PackageFlagTurboJsResultBridge |
                     venom::package::PackageFlagTurboJsFallbackPolicy |
                     venom::package::PackageFlagTurboJsEngineBackend;
  if ((flags & ~known) != 0u) emit("custom");
}
} // namespace

bool inspect_package(const std::filesystem::path& path) {
  const auto pkg = venom::package::read_package(path);

  std::cout << "VBC package\n"
            << "  file: " << path.string() << "\n"
            << "  version: " << pkg.version << "\n"
            << "  runtime_abi: " << pkg.runtime_abi << "\n"
            << "  flags: ";
  print_flags(pkg.flags);
  std::cout << "\n"
            << "  file_size: " << pkg.file_size << " bytes\n"
            << "  package_hash: " << hex64(pkg.package_hash) << "\n"
            << "  section_table: offset=" << pkg.section_table_offset << " size=" << pkg.section_table_size << "\n"
            << "  name_table: offset=" << pkg.name_table_offset << " size=" << pkg.name_table_size << "\n"
            << "  payload: offset=" << pkg.payload_offset << " size=" << pkg.payload_size << "\n"
            << "  sections: " << pkg.sections.size() << "\n";

  const auto print_section_flags = [](std::uint32_t flags) {
    if (flags == 0u) {
      std::cout << "none";
      return;
    }
    bool first = true;
    const auto emit = [&](const char* name) {
      if (!first) std::cout << "|";
      std::cout << name;
      first = false;
    };
    if ((flags & venom::package::SectionFlagCompressed) != 0u) emit("compressed");
    if ((flags & venom::package::SectionFlagEncrypted) != 0u) emit("encrypted");
    const auto known = venom::package::SectionFlagCompressed | venom::package::SectionFlagEncrypted;
    if ((flags & ~known) != 0u) emit("custom");
  };

  for (std::size_t i = 0; i < pkg.sections.size(); ++i) {
    const auto& section = pkg.sections[i];
    std::cout << "    [" << i << "] "
              << venom::package::section_type_name(section.type)
              << " name=\"" << section.name << "\""
              << " offset=" << section.offset
              << " raw_size=" << section.raw_size
              << " stored_size=" << section.stored_size
              << " hash=" << hex64(section.hash)
              << " flags=";
    print_section_flags(section.flags);
    std::cout << "\n";
  }

  return true;
}

} // namespace venom::compiler
