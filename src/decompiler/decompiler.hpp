#pragma once

#include "compiler/commands/cli.hpp"

#include <filesystem>

namespace venom::decompiler {

struct Options {
  std::filesystem::path input;
  std::filesystem::path output;
  std::filesystem::path key_file;
  venom::compiler::OutputFormat format = venom::compiler::OutputFormat::Text;
  bool recover_javascript = true;
  bool quickjs_disassembly = true;
  bool force = false;
};

bool recover(const Options& options);
int run_cli(int argc, char** argv);

} // namespace venom::decompiler
