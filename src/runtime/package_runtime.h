#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define VENOM_RUNTIME_ABI 1u
#define VENOM_PACKAGE_VERSION 40u

#define VENOM_RUNTIME_TEXT_CAP 4096u
#define VENOM_RUNTIME_ROUTE_CAP 128u

typedef struct VenomRuntimeReport {
  unsigned int package_version;
  unsigned int runtime_abi;
  unsigned int package_flags;
  unsigned int section_count;
  unsigned int route_count;
  unsigned int string_count;
  unsigned int total_instruction_count;
  unsigned int executed_instruction_count;
  unsigned int script_chunk_count;
  unsigned int asset_count;
  char selected_route[VENOM_RUNTIME_ROUTE_CAP];
  char rendered_text[VENOM_RUNTIME_TEXT_CAP];
} VenomRuntimeReport;

int venom_package_validate(const unsigned char* bytes, unsigned long size);
int venom_runtime_analyze(const unsigned char* bytes, unsigned long size, const char* route, VenomRuntimeReport* report);
unsigned int venom_runtime_abi(void);
unsigned int venom_package_version(void);

#ifdef __cplusplus
}
#endif
