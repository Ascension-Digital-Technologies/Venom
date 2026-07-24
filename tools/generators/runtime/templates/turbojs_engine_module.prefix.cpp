#include "pipeline/js.hpp"
#include "generated/contracts/turbojs_wasm_abi.hpp"

#include <stdexcept>
#include <sstream>

namespace venom::compiler {

std::string make_turbojs_engine_module_js(bool protected_release) {
  std::string js = R"TJSENGINE(