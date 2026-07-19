#pragma once

#include "venom/frontends/typescript/frontend.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

struct JSRuntime;
struct JSContext;

namespace venom::compiler::typescript {

struct EmbeddedRuntimeLimits {
  std::size_t heap_limit_bytes = 256ULL * 1024ULL * 1024ULL;
  std::size_t stack_limit_bytes = 4ULL * 1024ULL * 1024ULL;
  std::size_t source_limit_bytes = 32ULL * 1024ULL * 1024ULL;
  std::size_t garbage_collect_interval = 32;
};

class EmbeddedRuntime {
public:
  explicit EmbeddedRuntime(EmbeddedRuntimeLimits limits = {});
  ~EmbeddedRuntime();

  EmbeddedRuntime(const EmbeddedRuntime&) = delete;
  EmbeddedRuntime& operator=(const EmbeddedRuntime&) = delete;

  TranspileResult transpile(const std::string& source, const std::string& filename);
  const std::string& compiler_version() const noexcept { return compiler_version_; }
  const std::string& bootstrap_mode() const noexcept { return bootstrap_mode_; }
  double initialization_milliseconds() const noexcept { return initialization_milliseconds_; }

private:
  void initialize();
  void evaluate_script(const std::string& source, const char* filename);
  void evaluate_bytecode();
  void initialize_from_source();
  std::string exception_message() const;

  EmbeddedRuntimeLimits limits_;
  JSRuntime* runtime_ = nullptr;
  JSContext* context_ = nullptr;
  void* transpile_function_opaque_ = nullptr;
  std::string compiler_version_;
  std::string bootstrap_mode_;
  double initialization_milliseconds_ = 0.0;
  std::size_t call_count_ = 0;
};

class EmbeddedFrontendService {
public:
  static EmbeddedFrontendService& instance();
  TranspileResult transpile(const std::string& source, const std::string& filename);
  std::string compiler_version();

private:
  EmbeddedFrontendService() = default;
  std::mutex mutex_;
  std::unique_ptr<EmbeddedRuntime> runtime_;
};

} // namespace venom::compiler::typescript
