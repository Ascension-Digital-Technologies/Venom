#pragma once

#include "../v4m3/vm.hpp"
#include <string>

namespace venom::v4m5 {

// Deliberately strict subset of JavaScript, compiled without eval or a JS engine:
// function main() { let/const; assignment; if/else; while; return; }
// Number values must remain in signed int32 range for checked +, -, *.
// Also supports ^, ===, !==, <, <=, >, >=, !, boolean &&/||\n// with true short-circuit behavior, and the host(cap, arg) intrinsic.
// No calls other than host, objects, coercions, closures, general JS numbers,
// floating point, string literals, general JavaScript coercions or DOM.
// The compiler rejects unsupported syntax or static type mismatches.
v4m3::Program compile_source(const std::string& source);

} // namespace venom::v4m5
