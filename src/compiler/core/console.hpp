#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace venom::compiler {

struct BuildOptions;

class Console {
 public:
  explicit Console(const BuildOptions& options);

  void banner(std::string_view title, std::string_view subtitle) const;
  void field(std::string_view label, const std::string& value) const;
  void phase(std::string_view message);
  void info(std::string_view message) const;
  void detail(std::string_view message) const;
  void warning(std::string_view message) const;
  void success(std::string_view message) const;
  void summary(std::string_view label, const std::string& value) const;
  void artifact(std::string_view label, const std::filesystem::path& path) const;
  void write_performance_report(const std::filesystem::path& path, const std::string& cache_json);
  long long elapsed_milliseconds() const;

  bool enabled() const { return enabled_; }
  bool detailed() const { return detailed_; }

 private:
  using Clock = std::chrono::steady_clock;
  std::string paint(std::string_view code, std::string_view value) const;
  static std::string format_duration(long long milliseconds);
  void pace_after_phase();

  bool enabled_ = true;
  bool detailed_ = false;
  bool color_ = false;
  bool paced_ = false;
  Clock::time_point started_;
  Clock::time_point last_;
  std::chrono::milliseconds pacing_time_{0};
  unsigned phase_index_ = 0;
  struct PhaseTiming { std::string name; long long milliseconds = 0; };
  std::vector<PhaseTiming> phase_timings_;
  std::string current_phase_;
  unsigned phase_total_ = 18;
};

void print_status(std::string_view label, std::string_view message);
void print_pass(std::string_view message);
void print_fatal_error(std::string_view message);

} // namespace venom::compiler
