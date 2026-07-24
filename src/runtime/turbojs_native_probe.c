#include "runtime/turbojs_upstream_runtime.h"

#include <stdio.h>

int main(void) {
  VenomTurboJsNativeReport report;
  const int rc = venom_tjs_native_run_parity_smoke(&report);
  printf("venom_tjs_native_probe\n");
  printf("  available: %s\n", report.available ? "yes" : "no");
  printf("  runtime_abi: %u\n", report.runtime_abi);
  printf("  package_version: %u\n", report.package_version);
  printf("  checks_passed: %u\n", report.checks_passed);
  printf("  checks_failed: %u\n", report.checks_failed);
  printf("  result_hash32: %08x\n", report.result_hash32);
  if (report.result_json[0]) {
    printf("  result: %s\n", report.result_json);
  }
  if (report.exception[0]) {
    printf("  exception: %s\n", report.exception);
  }
  return rc;
}
