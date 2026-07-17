#include "package_runtime.h"

#include <stdio.h>
#include <stdlib.h>

static unsigned char* read_file(const char* path, unsigned long* out_size) {
  FILE* file = NULL;
#if defined(_WIN32)
  if (fopen_s(&file, path, "rb") != 0) file = NULL;
#else
  file = fopen(path, "rb");
#endif
  if (!file) return NULL;
  if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return NULL; }
  long size = ftell(file);
  if (size < 0) { fclose(file); return NULL; }
  if (fseek(file, 0, SEEK_SET) != 0) { fclose(file); return NULL; }
  unsigned char* bytes = (unsigned char*)malloc((size_t)(size ? size : 1));
  if (!bytes) { fclose(file); return NULL; }
  if (size > 0 && fread(bytes, 1u, (size_t)size, file) != (size_t)size) {
    free(bytes);
    fclose(file);
    return NULL;
  }
  fclose(file);
  *out_size = (unsigned long)size;
  return bytes;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: venom_runtime_probe <app.vbc> [route]\n");
    return 2;
  }
  unsigned long size = 0;
  unsigned char* bytes = read_file(argv[1], &size);
  if (!bytes) {
    fprintf(stderr, "failed to read %s\n", argv[1]);
    return 2;
  }

  VenomRuntimeReport report;
  const int rc = venom_runtime_analyze(bytes, size, argc >= 3 ? argv[2] : "/", &report);
  free(bytes);
  if (rc != 0) {
    fprintf(stderr, "venom runtime probe failed: %d\n", rc);
    return rc;
  }

  printf("venom_runtime_probe\n");
  printf("  version: %u\n", report.package_version);
  printf("  runtime_abi: %u\n", report.runtime_abi);
  printf("  sections: %u\n", report.section_count);
  printf("  routes: %u\n", report.route_count);
  printf("  strings: %u\n", report.string_count);
  printf("  instructions: %u\n", report.total_instruction_count);
  printf("  executed: %u\n", report.executed_instruction_count);
  printf("  scripts: %u\n", report.script_chunk_count);
  printf("  assets: %u\n", report.asset_count);
  printf("  route: %s\n", report.selected_route);
  printf("  text: %s\n", report.rendered_text);
  return 0;
}
