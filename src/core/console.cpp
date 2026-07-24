#include "core/console.hpp"
#include "core/options.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace venom::compiler {
namespace {
thread_local unsigned active_phase_index = 0;
thread_local unsigned active_phase_total = 0;
thread_local std::string active_phase_message;

bool stdout_is_terminal() {
#if defined(_WIN32)
  return _isatty(_fileno(stdout)) != 0;
#else
  return ::isatty(STDOUT_FILENO) != 0;
#endif
}

void enable_windows_ansi() {
#if defined(_WIN32)
  const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (handle == INVALID_HANDLE_VALUE) return;
  DWORD mode = 0;
  if (GetConsoleMode(handle, &mode)) SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}
}

Console::Console(const BuildOptions& options)
    : enabled_(options.format != OutputFormat::Json && options.verbosity > 0),
      detailed_(options.format != OutputFormat::Json && options.verbosity > 1),
      started_(Clock::now()), last_(started_) {
  const bool interactive = stdout_is_terminal();
  color_ = interactive && std::getenv("NO_COLOR") == nullptr;
  paced_ = interactive;
  if (color_) enable_windows_ansi();
}

std::string Console::paint(std::string_view code, std::string_view value) const {
  if (!color_) return std::string(value);
  return "\x1b[" + std::string(code) + "m" + std::string(value) + "\x1b[0m";
}

std::string Console::format_duration(long long milliseconds) {
  std::ostringstream out;
  if (milliseconds >= 1000) out << std::fixed << std::setprecision(2) << static_cast<double>(milliseconds) / 1000.0 << "s";
  else out << milliseconds << "ms";
  return out.str();
}

void Console::banner(std::string_view title, std::string_view subtitle) const {
  if (!enabled_) return;
  std::cout << "\n" << paint("1;35", "VENOM") << "  " << paint("1", title) << "\n"
            << "       " << subtitle << "\n"
            << paint("2", "------------------------------------------------------------------------") << "\n";
}

void Console::field(std::string_view label, const std::string& value) const {
  if (!enabled_) return;
  std::cout << paint("1;36", "[CONFIG]") << "   " << std::left << std::setw(12) << label << value << "\n";
}

void Console::pace_after_phase() {
  if (!paced_ || phase_index_ == 0) return;
  constexpr auto delay = std::chrono::seconds(1);
  std::this_thread::sleep_for(delay);
  pacing_time_ += std::chrono::duration_cast<std::chrono::milliseconds>(delay);
}

void Console::phase(std::string_view message) {
  const auto completed_at = Clock::now();
  const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(completed_at - last_).count();
  if (phase_index_ > 0) {
    phase_timings_.push_back({current_phase_, delta});
    if (enabled_ && detailed_) {
      std::cout << paint("2", "[DETAIL]") << "   Previous step completed in "
                << format_duration(delta) << "\n";
    }
  }
  if (enabled_) {
    // Keep fast phases readable in an interactive terminal. The delay is
    // presentation-only and is excluded from reported build timings.
    pace_after_phase();
  }
  ++phase_index_;
  active_phase_index = phase_index_;
  active_phase_total = phase_total_;
  active_phase_message = std::string(message);
  current_phase_ = std::string(message);
  if (enabled_) {
    std::ostringstream tag;
    tag << "[" << phase_index_ << "/" << phase_total_ << "]";
    std::cout << "\n" << paint("1;34", tag.str()) << " " << paint("1", message)
              << "\n" << std::flush;
  }
  last_ = Clock::now();
}
void Console::info(std::string_view message) const {
  if (enabled_) std::cout << paint("36", "[INFO]") << "     " << message << "\n";
}

void Console::detail(std::string_view message) const {
  if (detailed_) std::cout << paint("2", "[DETAIL]") << "   " << message << "\n";
}

void Console::warning(std::string_view message) const {
  if (enabled_) std::cout << paint("1;33", "[WARN]") << "     " << message << "\n";
}

void Console::success(std::string_view message) const {
  if (!enabled_) return;
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started_);
  elapsed -= pacing_time_;
  if (paced_ && phase_index_ > 0) {
    constexpr auto delay = std::chrono::seconds(1);
    std::this_thread::sleep_for(delay);
  }
  if (elapsed.count() < 0) elapsed = std::chrono::milliseconds{0};
  std::cout << "\n" << paint("1;32", "[SUCCESS]") << "  " << message << " "
            << paint("2", "(" + format_duration(elapsed.count()) + ")") << "\n";
}

void Console::summary(std::string_view label, const std::string& value) const {
  if (!enabled_) return;
  std::cout << "  " << paint("2", std::string(label) + ":") << " " << value << "\n";
}

void Console::artifact(std::string_view label, const std::filesystem::path& path) const {
  summary(label, path.generic_string());
}

long long Console::elapsed_milliseconds() const {
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started_) - pacing_time_;
  return std::max<long long>(0, elapsed.count());
}

void Console::write_performance_report(const std::filesystem::path& path, const std::string& cache_json) {
  const auto now = Clock::now();
  if (phase_index_ > phase_timings_.size() && !current_phase_.empty()) {
    const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_).count();
    phase_timings_.push_back({current_phase_, delta});
  }
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return;
  auto escape = [](const std::string& value) {
    std::string result;
    for (char c : value) {
      if (c == '\\' || c == '"') result.push_back('\\');
      if (c == '\n') result += "\\n"; else result.push_back(c);
    }
    return result;
  };
  out << "{\n  \"schema_version\": 1,\n  \"elapsed_ms\": " << elapsed_milliseconds()
      << ",\n  \"phases\": [\n";
  for (std::size_t i = 0; i < phase_timings_.size(); ++i) {
    out << "    {\"index\": " << (i + 1) << ", \"name\": \"" << escape(phase_timings_[i].name)
        << "\", \"milliseconds\": " << phase_timings_[i].milliseconds << "}"
        << (i + 1 == phase_timings_.size() ? "\n" : ",\n");
  }
  out << "  ],\n  \"cache\": " << cache_json << "\n}\n";
}

void print_status(std::string_view label, std::string_view message) {
  const bool color = stdout_is_terminal() && std::getenv("NO_COLOR") == nullptr;
  if (color) enable_windows_ansi();
  std::string code = "36";
  if (label == "PASS" || label == "SUCCESS") code = "1;32";
  else if (label == "WARN") code = "1;33";
  else if (label == "ERROR" || label == "FAIL") code = "1;31";
  else if (label == "SERVER") code = "1;35";
  const std::string tag = "[" + std::string(label) + "]";
  std::cout << (color ? "\x1b[" + code + "m" + tag + "\x1b[0m" : tag)
            << std::string(tag.size() < 8 ? 8 - tag.size() : 1, ' ') << message << '\n' << std::flush;
}

void print_pass(std::string_view message) { print_status("PASS", message); }

ConsoleContext current_console_context() {
  return {active_phase_index, active_phase_total, active_phase_message};
}

} // namespace venom::compiler
