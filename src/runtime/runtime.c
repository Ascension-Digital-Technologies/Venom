#include "runtime.h"
#include "package_runtime.h"

int venom_runtime_boot(const unsigned char* package_bytes, unsigned long package_size) {
  return venom_package_validate(package_bytes, package_size);
}

unsigned int venom_runtime_mode_js(void) { return 1u; }
unsigned int venom_runtime_mode_wasm(void) { return 2u; }
