#include "venom/pipeline/js.hpp"
#include "venom/generated/contracts/quickjs_wasm_abi.hpp"

#include <stdexcept>
#include <sstream>

namespace venom::compiler {

std::string make_quickjs_engine_module_js(bool protected_release) {
  std::string js = R"QJSENGINE(