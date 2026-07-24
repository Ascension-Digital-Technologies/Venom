#pragma once

#include <iosfwd>

namespace venom::compiler {

// Development-only helper used by the local JavaScript playground. Reads
// JavaScript source from stdin and writes a JSON envelope containing native
// TurboJS bytecode to stdout.
bool compile_snippet(std::istream& input, std::ostream& output);

} // namespace venom::compiler
