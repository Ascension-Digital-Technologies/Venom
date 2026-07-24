#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VenomTurboJsNativeReport {
  uint32_t available;
  uint32_t runtime_abi;
  uint32_t package_version;
  uint32_t checks_passed;
  uint32_t checks_failed;
  uint32_t result_hash32;
  char result_json[512];
  char exception[512];
} VenomTurboJsNativeReport;

uint32_t venom_tjs_native_available(void);
int venom_tjs_native_run_parity_smoke(VenomTurboJsNativeReport* report);

#ifdef __cplusplus
}
#endif
