#include "venom/core/process.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <limits>
#include <string>
#include <vector>
#else
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace venom::compiler {

#ifdef _WIN32
namespace {

std::wstring widen_windows_argument(const std::string& value) {
  if (value.empty()) return {};

  auto convert = [&](UINT code_page, DWORD flags) -> std::wstring {
    const int required = MultiByteToWideChar(
      code_page,
      flags,
      value.data(),
      static_cast<int>(value.size()),
      nullptr,
      0);
    if (required <= 0) return {};

    std::wstring wide(static_cast<std::size_t>(required), L'\0');
    const int written = MultiByteToWideChar(
      code_page,
      flags,
      value.data(),
      static_cast<int>(value.size()),
      wide.data(),
      required);
    if (written != required) return {};
    return wide;
  };

  // Venom uses UTF-8 internally. Fall back to the active Windows code page for
  // paths supplied by older tooling that still exposes ANSI strings.
  auto wide = convert(CP_UTF8, MB_ERR_INVALID_CHARS);
  if (!wide.empty()) return wide;
  wide = convert(CP_ACP, 0);
  return wide;
}

std::wstring quote_windows_argument(const std::wstring& argument) {
  if (argument.empty()) return L"\"\"";

  bool requires_quotes = false;
  for (const wchar_t value : argument) {
    if (value == L'"' || value == L' ' || value == L'\t' || value == L'\n' || value == L'\v') {
      requires_quotes = true;
      break;
    }
  }
  if (!requires_quotes) return argument;

  // Follow the CommandLineToArgvW/MSVC parsing contract. Backslashes are only
  // special when they precede a quote or the closing quote.
  std::wstring quoted;
  quoted.push_back(L'"');
  std::size_t backslashes = 0;
  for (const wchar_t value : argument) {
    if (value == L'\\') {
      ++backslashes;
      continue;
    }
    if (value == L'"') {
      quoted.append(backslashes * 2u + 1u, L'\\');
      quoted.push_back(L'"');
      backslashes = 0;
      continue;
    }
    quoted.append(backslashes, L'\\');
    backslashes = 0;
    quoted.push_back(value);
  }
  quoted.append(backslashes * 2u, L'\\');
  quoted.push_back(L'"');
  return quoted;
}

int windows_error_result(DWORD error) {
  if (error == 0) return -1;
  const auto bounded = error > static_cast<DWORD>(std::numeric_limits<int>::max())
    ? std::numeric_limits<int>::max()
    : static_cast<int>(error);
  return -bounded;
}

} // namespace
#endif

int run_process(const std::string& executable, const std::vector<std::string>& arguments) {
#ifdef _WIN32
  const auto wide_executable = widen_windows_argument(executable);
  if (wide_executable.empty()) return windows_error_result(ERROR_NO_UNICODE_TRANSLATION);

  std::wstring command_line = quote_windows_argument(wide_executable);
  for (const auto& argument : arguments) {
    const auto wide_argument = widen_windows_argument(argument);
    if (!argument.empty() && wide_argument.empty()) {
      return windows_error_result(ERROR_NO_UNICODE_TRANSLATION);
    }
    command_line.push_back(L' ');
    command_line += quote_windows_argument(wide_argument);
  }

  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION process{};

  // CreateProcessW requires a mutable, null-terminated command-line buffer.
  const BOOL started = CreateProcessW(
    nullptr,
    command_line.data(),
    nullptr,
    nullptr,
    FALSE,
    0,
    nullptr,
    nullptr,
    &startup,
    &process);
  if (!started) return windows_error_result(GetLastError());

  const DWORD wait_result = WaitForSingleObject(process.hProcess, INFINITE);
  if (wait_result != WAIT_OBJECT_0) {
    const DWORD error = wait_result == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return windows_error_result(error);
  }

  DWORD exit_code = 0;
  if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
    const DWORD error = GetLastError();
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return windows_error_result(error);
  }

  CloseHandle(process.hThread);
  CloseHandle(process.hProcess);
  if (exit_code > static_cast<DWORD>(std::numeric_limits<int>::max())) {
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(exit_code);
#else
  const pid_t pid = fork();
  if (pid < 0) {
    return -errno;
  }
  if (pid == 0) {
    std::vector<char*> argv;
    argv.reserve(arguments.size() + 2u);
    argv.push_back(const_cast<char*>(executable.c_str()));
    for (const auto& argument : arguments) {
      argv.push_back(const_cast<char*>(argument.c_str()));
    }
    argv.push_back(nullptr);
    execvp(executable.c_str(), argv.data());
    _exit(127);
  }

  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) {
      return -errno;
    }
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return -1;
#endif
}

} // namespace venom::compiler
