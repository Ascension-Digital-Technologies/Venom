#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int venom_runtime_boot(const unsigned char* package_bytes, unsigned long package_size);
unsigned int venom_runtime_mode_js(void);
unsigned int venom_runtime_mode_wasm(void);

#ifdef __cplusplus
}
#endif
