#include "package_runtime.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define VENOM_HEADER_SIZE 80u
#define VENOM_SECTION_ENTRY_SIZE 48u
#define VENOM_PACKAGE_HASH_OFFSET 72u

#define VENOM_PACKAGE_FLAG_PROTECT 1u
#define VENOM_PACKAGE_FLAG_RELEASE 2u
#define VENOM_PACKAGE_FLAG_POLYMORPHIC 4u
#define VENOM_PACKAGE_FLAG_COMPRESSED 8u
#define VENOM_PACKAGE_FLAG_CRYPTO_READY 16u
#define VENOM_PACKAGE_FLAG_INTEGRITY_METADATA 32u
#define VENOM_PACKAGE_FLAG_AEAD_READY 64u
#define VENOM_PACKAGE_FLAG_RUNTIME_HARDENED 128u
#define VENOM_PACKAGE_FLAG_WASM_RUNTIME 256u
#define VENOM_PACKAGE_FLAG_HOST_BRIDGE 512u
#define VENOM_PACKAGE_FLAG_BINARY_DOM_OPS 1024u
#define VENOM_PACKAGE_FLAG_FETCH_BRIDGE 2048u
#define VENOM_PACKAGE_FLAG_ASYNC_HOST_QUEUE 4096u
#define VENOM_PACKAGE_FLAG_TIMER_BRIDGE 8192u
#define VENOM_PACKAGE_FLAG_EVENT_QUEUE 16384u
#define VENOM_PACKAGE_FLAG_QUICKJS_BRIDGE 32768u
#define VENOM_PACKAGE_FLAG_SCRIPT_ISOLATION 65536u
#define VENOM_PACKAGE_FLAG_SCRIPT_POLICY 131072u
#define VENOM_PACKAGE_FLAG_QUICKJS_CHUNKS 262144u
#define VENOM_PACKAGE_FLAG_QUICKJS_ENGINE 524288u
#define VENOM_PACKAGE_FLAG_SCRIPT_ENGINE_FALLBACK 1048576u
#define VENOM_PACKAGE_FLAG_QUICKJS_ENGINE_MODULE 2097152u
#define VENOM_PACKAGE_FLAG_QUICKJS_CONTEXT_LIFECYCLE 4194304u
#define VENOM_PACKAGE_FLAG_HOST_CAPABILITIES 8388608u
#define VENOM_PACKAGE_FLAG_QUICKJS_ADAPTER_DIAGNOSTICS 16777216u
#define VENOM_PACKAGE_FLAG_QUICKJS_WASM_RUNTIME 33554432u
#define VENOM_PACKAGE_FLAG_QUICKJS_SOURCE_TRANSFER 67108864u
#define VENOM_PACKAGE_FLAG_QUICKJS_CONSOLE_BRIDGE 134217728u
#define VENOM_PACKAGE_FLAG_QUICKJS_EXECUTION_RECORDS 268435456u
#define VENOM_PACKAGE_FLAG_QUICKJS_RESULT_BRIDGE 536870912u
#define VENOM_PACKAGE_FLAG_QUICKJS_FALLBACK_POLICY 1073741824u
#define VENOM_PACKAGE_FLAG_QUICKJS_ENGINE_BACKEND 2147483648u

#define VENOM_SECTION_COMPRESSED 1u
#define VENOM_SECTION_ENCRYPTED 2u

#define VENOM_SECTION_MANIFEST 1u
#define VENOM_SECTION_ROUTES 2u
#define VENOM_SECTION_DOM_TEMPLATES 3u
#define VENOM_SECTION_CSS 4u
#define VENOM_SECTION_JAVASCRIPT 5u
#define VENOM_SECTION_QUICKJS 6u
#define VENOM_SECTION_VM_BYTECODE 7u
#define VENOM_SECTION_ASSET 8u
#define VENOM_SECTION_INTEGRITY 9u
#define VENOM_SECTION_STRINGS 10u
#define VENOM_SECTION_ASSET_MANIFEST 11u
#define VENOM_SECTION_HOST_BRIDGE 12u
#define VENOM_SECTION_FETCH_BRIDGE 13u
#define VENOM_SECTION_ASYNC_HOST_QUEUE 14u
#define VENOM_SECTION_TIMER_BRIDGE 15u
#define VENOM_SECTION_EVENT_QUEUE 16u
#define VENOM_SECTION_QUICKJS_BRIDGE 17u
#define VENOM_SECTION_SCRIPT_ISOLATION 18u
#define VENOM_SECTION_SCRIPT_POLICY 19u
#define VENOM_SECTION_QUICKJS_CHUNKS 20u
#define VENOM_SECTION_QUICKJS_ENGINE 21u
#define VENOM_SECTION_SCRIPT_ENGINE_POLICY 22u
#define VENOM_SECTION_QUICKJS_ENGINE_MODULE 23u
#define VENOM_SECTION_QUICKJS_CONTEXT_LIFECYCLE 24u
#define VENOM_SECTION_HOST_CAPABILITIES 25u
#define VENOM_SECTION_QUICKJS_ADAPTER_DIAGNOSTICS 26u
#define VENOM_SECTION_QUICKJS_WASM_RUNTIME 27u
#define VENOM_SECTION_QUICKJS_SOURCE_TRANSFER 28u
#define VENOM_SECTION_QUICKJS_CONSOLE_BRIDGE 29u
#define VENOM_SECTION_QUICKJS_EXECUTION_RECORDS 30u
#define VENOM_SECTION_QUICKJS_RESULT_BRIDGE 31u
#define VENOM_SECTION_QUICKJS_FALLBACK_POLICY 32u
#define VENOM_SECTION_QUICKJS_ENGINE_BACKEND 33u
#define VENOM_SECTION_QUICKJS_NATIVE_PARITY 34u
#define VENOM_SECTION_QUICKJS_EXECUTION_MODE 35u
#define VENOM_SECTION_QUICKJS_RUNTIME_ABI 36u
#define VENOM_SECTION_QUICKJS_HOST_IMPORTS 37u
#define VENOM_SECTION_QUICKJS_HEAP_LIMITS 38u
#define VENOM_SECTION_QUICKJS_SCRIPT_BUFFER 39u
#define VENOM_SECTION_QUICKJS_CONSOLE_ABI 40u
#define VENOM_SECTION_QUICKJS_PARITY_PROBE 41u
#define VENOM_SECTION_QUICKJS_RELEASE_FALLBACK 42u
#define VENOM_SECTION_QUICKJS_BYTECODE_MANIFEST 43u
#define VENOM_SECTION_QUICKJS_MODULE_RESOLVER 44u
#define VENOM_SECTION_QUICKJS_EXCEPTION_ABI 45u
#define VENOM_SECTION_QUICKJS_HOST_TRAP_POLICY 46u
#define VENOM_SECTION_QUICKJS_EXECUTION_LIFECYCLE 47u
#define VENOM_SECTION_QUICKJS_SCRIPT_BUFFER_POLICY 48u
#define VENOM_SECTION_QUICKJS_CONTEXT_LIMIT_POLICY 49u
#define VENOM_SECTION_QUICKJS_HOST_CALL_DISPATCH 50u
#define VENOM_SECTION_QUICKJS_PARITY_CONTRACT 51u
#define VENOM_SECTION_QUICKJS_RELEASE_FAILCLOSED 52u
#define VENOM_SECTION_QUICKJS_MODULE_GRAPH 53u
#define VENOM_SECTION_QUICKJS_MODULE_EXECUTION 54u
#define VENOM_SECTION_QUICKJS_MODULE_CACHE 55u
#define VENOM_SECTION_QUICKJS_RESOLVER_AUDIT 56u
#define VENOM_SECTION_QUICKJS_INTEROP_FALLBACK 57u
#define VENOM_SECTION_QUICKJS_EXECUTION_PIPELINE 58u
#define VENOM_SECTION_QUICKJS_SCRIPT_RECORDS 59u
#define VENOM_SECTION_QUICKJS_EVAL_RESULTS 60u
#define VENOM_SECTION_QUICKJS_CONSOLE_CAPTURE 61u
#define VENOM_SECTION_QUICKJS_FAILURE_REPORTS 62u
#define VENOM_SECTION_QUICKJS_EXECUTION_JOURNAL 63u
#define VENOM_SECTION_QUICKJS_CHECKPOINT_POLICY 64u
#define VENOM_SECTION_QUICKJS_REPLAY_CURSOR 65u
#define VENOM_SECTION_QUICKJS_RESUME_STATE 66u
#define VENOM_SECTION_QUICKJS_DETERMINISM_AUDIT 67u
#define VENOM_SECTION_QUICKJS_SNAPSHOT_POLICY 68u
#define VENOM_SECTION_QUICKJS_SNAPSHOT_RECORDS 69u
#define VENOM_SECTION_QUICKJS_REPLAY_VALIDATION 70u
#define VENOM_SECTION_QUICKJS_DETERMINISM_LEDGER 71u
#define VENOM_SECTION_QUICKJS_AUDIT_SEAL 72u
#define VENOM_SECTION_QUICKJS_EXECUTION_COMMIT 73u
#define VENOM_SECTION_QUICKJS_ROLLBACK_POLICY 74u
#define VENOM_SECTION_QUICKJS_HOST_CALL_RECEIPTS 75u
#define VENOM_SECTION_QUICKJS_RELEASE_ACCEPTANCE 76u
#define VENOM_SECTION_QUICKJS_COMMIT_AUDIT 77u
#define VENOM_SECTION_QUICKJS_CAPABILITY_POLICY 78u
#define VENOM_SECTION_QUICKJS_HOST_IO_POLICY 79u
#define VENOM_SECTION_QUICKJS_PERMISSION_SEAL 80u
#define VENOM_SECTION_QUICKJS_POLICY_RECEIPTS 81u
#define VENOM_SECTION_QUICKJS_RELEASE_GATE 83u
#define VENOM_SECTION_QUICKJS_HOST_IO_DECISION 83u
#define VENOM_SECTION_QUICKJS_HOST_IO_DENY_TRACE 84u
#define VENOM_SECTION_QUICKJS_CAPABILITY_LEDGER 85u
#define VENOM_SECTION_QUICKJS_POLICY_SEAL_AUDIT 86u
#define VENOM_SECTION_QUICKJS_RUNTIME_DENYLIST 87u
#define VENOM_SECTION_PACKAGE_FEATURES 88u

#define VENOM_LOGICAL_NOP 0u
#define VENOM_LOGICAL_LOAD_CONST 1u
#define VENOM_LOGICAL_CREATE_ELEMENT 2u
#define VENOM_LOGICAL_SET_ATTRIBUTE 3u
#define VENOM_LOGICAL_SET_TEXT 4u
#define VENOM_LOGICAL_APPEND_CHILD 5u
#define VENOM_LOGICAL_ENTER_ELEMENT 6u
#define VENOM_LOGICAL_LEAVE_ELEMENT 7u
#define VENOM_LOGICAL_CALL_QUICKJS 8u
#define VENOM_LOGICAL_HALT 9u

#define VENOM_WORD_OPCODE 0u
#define VENOM_WORD_A 1u
#define VENOM_WORD_B 2u
#define VENOM_WORD_C 3u

#define VENOM_OK 0
#define VENOM_ERR_ARGUMENT 1
#define VENOM_ERR_TRUNCATED 2
#define VENOM_ERR_MAGIC 3
#define VENOM_ERR_VERSION 4
#define VENOM_ERR_ABI 5
#define VENOM_ERR_FLAGS 6
#define VENOM_ERR_RANGE 7
#define VENOM_ERR_HASH 8
#define VENOM_ERR_SECTION 9
#define VENOM_ERR_COMPRESS 10
#define VENOM_ERR_FORMAT 11
#define VENOM_ERR_MISSING 12
#define VENOM_ERR_VM 13

struct VenomBytes {
  unsigned char* data;
  uint64_t size;
};

struct VenomSection {
  uint32_t type;
  uint32_t flags;
  const unsigned char* name;
  uint32_t name_size;
  uint64_t offset;
  uint64_t stored_size;
  uint64_t raw_size;
  uint64_t hash;
  struct VenomBytes decoded;
};

struct VenomPackageImage {
  const unsigned char* bytes;
  uint64_t size;
  uint32_t version;
  uint32_t abi;
  uint32_t flags;
  uint32_t section_count;
  uint64_t section_table_offset;
  uint64_t section_table_size;
  uint64_t name_table_offset;
  uint64_t name_table_size;
  uint64_t payload_offset;
  uint64_t payload_size;
  uint64_t package_hash;
  struct VenomSection* sections;
};

struct VenomStringRef {
  const unsigned char* data;
  uint32_t size;
};

struct VenomStringTable {
  struct VenomStringRef* strings;
  unsigned char* decoded_data;
  uint32_t count;
};

struct VenomRouteEntry {
  uint32_t route_id;
  uint32_t source_id;
  uint32_t bytecode_offset;
  uint32_t bytecode_size;
  uint32_t instruction_count;
  uint32_t flags;
};

struct VenomRouteTable {
  struct VenomRouteEntry* routes;
  uint32_t count;
};

struct VenomVmBytecode {
  const unsigned char* data;
  uint32_t data_size;
  uint32_t instruction_size;
  uint32_t instruction_count;
  uint32_t flags;
};

struct VenomPolyMap {
  uint32_t physical_to_logical[65536];
  uint32_t has_physical[65536 / 32];
  uint32_t word_layout[4];
  uint32_t opcode_mask;
  uint32_t operand_masks[3];
};

static unsigned int read_u32_le(const unsigned char* bytes) {
  return ((unsigned int)bytes[0]) |
         ((unsigned int)bytes[1] << 8u) |
         ((unsigned int)bytes[2] << 16u) |
         ((unsigned int)bytes[3] << 24u);
}

static uint64_t read_u64_le(const unsigned char* bytes) {
  uint64_t value = 0;
  for (unsigned int i = 0; i < 8u; ++i) {
    value |= ((uint64_t)bytes[i]) << (i * 8u);
  }
  return value;
}

static uint64_t fnv1a64_update(uint64_t hash, const unsigned char* bytes, uint64_t size) {
  const uint64_t prime = UINT64_C(1099511628211);
  for (uint64_t i = 0; i < size; ++i) {
    hash ^= (uint64_t)bytes[i];
    hash *= prime;
  }
  return hash;
}

static uint64_t fnv1a64_bytes(const unsigned char* bytes, uint64_t size) {
  return fnv1a64_update(UINT64_C(1469598103934665603), bytes, size);
}

static uint64_t fnv1a64_package(const unsigned char* bytes, uint64_t size) {
  uint64_t hash = UINT64_C(1469598103934665603);
  if (size <= VENOM_PACKAGE_HASH_OFFSET) {
    return fnv1a64_bytes(bytes, size);
  }
  hash = fnv1a64_update(hash, bytes, VENOM_PACKAGE_HASH_OFFSET);
  static const unsigned char zeros[8] = {0,0,0,0,0,0,0,0};
  hash = fnv1a64_update(hash, zeros, 8u);
  if (size > VENOM_PACKAGE_HASH_OFFSET + 8u) {
    hash = fnv1a64_update(hash, bytes + VENOM_PACKAGE_HASH_OFFSET + 8u, size - VENOM_PACKAGE_HASH_OFFSET - 8u);
  }
  return hash;
}


static uint64_t fnv1a64_update_u64(uint64_t hash, uint64_t value) {
  unsigned char bytes[8];
  for (unsigned int i = 0; i < 8u; ++i) bytes[i] = (unsigned char)((value >> (i * 8u)) & 0xffu);
  return fnv1a64_update(hash, bytes, 8u);
}

static uint64_t seal_mix64(uint64_t value) {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value = (value ^ (value >> 30u)) * UINT64_C(0xbf58476d1ce4e5b9);
  value = (value ^ (value >> 27u)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31u);
}

static uint64_t seal_derive_seed(uint32_t section_type, uint64_t name_digest, uint64_t plain_size, uint64_t plain_hash) {
  uint64_t h = UINT64_C(1469598103934665603);
  h = fnv1a64_update_u64(h, (uint64_t)section_type);
  h = fnv1a64_update_u64(h, name_digest);
  h = fnv1a64_update_u64(h, plain_size);
  h = fnv1a64_update_u64(h, plain_hash);
  h ^= UINT64_C(0x7f4a7c159e3779b9);
  h = seal_mix64(h ^ UINT64_C(0xd6e8feb86659fd93));
  return h == 0u ? UINT64_C(0x7f4a7c159e3779b9) : h;
}

static uint64_t aead_stream_seed(uint32_t section_type, uint64_t name_digest, uint64_t plain_size, uint64_t nonce_a, uint64_t nonce_b) {
  uint64_t h = UINT64_C(1469598103934665603) ^ UINT64_C(0xa41bc927f35d69e1);
  h = fnv1a64_update_u64(h, (uint64_t)section_type);
  h = fnv1a64_update_u64(h, name_digest);
  h = fnv1a64_update_u64(h, plain_size);
  h = fnv1a64_update_u64(h, nonce_a);
  h = fnv1a64_update_u64(h, nonce_b);
  return seal_mix64(h ^ UINT64_C(0x6c8e9cf570932bd5));
}

static void aead_tag(uint32_t section_type, uint64_t name_digest, uint64_t plain_size, uint64_t nonce_a, uint64_t nonce_b, const unsigned char* ciphertext, uint64_t ciphertext_size, uint64_t* out_a, uint64_t* out_b) {
  uint64_t a = UINT64_C(1469598103934665603) ^ UINT64_C(0x3d6f0a9b8c71e245);
  a = fnv1a64_update_u64(a, (uint64_t)section_type);
  a = fnv1a64_update_u64(a, name_digest);
  a = fnv1a64_update_u64(a, plain_size);
  a = fnv1a64_update_u64(a, nonce_a);
  a = fnv1a64_update_u64(a, nonce_b);
  a = fnv1a64_update(a, ciphertext, ciphertext_size);
  a = seal_mix64(a ^ UINT64_C(0x9b62d14fa7c8305d));

  uint64_t b = UINT64_C(1469598103934665603) ^ UINT64_C(0x6c8e9cf570932bd5);
  b = fnv1a64_update_u64(b, nonce_b);
  b = fnv1a64_update_u64(b, nonce_a);
  b = fnv1a64_update_u64(b, plain_size);
  b = fnv1a64_update_u64(b, name_digest);
  b = fnv1a64_update_u64(b, (uint64_t)section_type);
  b = fnv1a64_update(b, ciphertext, ciphertext_size);
  b = seal_mix64(b ^ UINT64_C(0xa41bc927f35d69e1));
  *out_a = a;
  *out_b = b;
}

static uint64_t seal_next_word(uint64_t* state) {
  *state += UINT64_C(0x9e3779b97f4a7c15);
  return seal_mix64(*state);
}


static void write_u32_le(unsigned char* out, uint32_t value) {
  out[0] = (unsigned char)(value & 0xffu);
  out[1] = (unsigned char)((value >> 8u) & 0xffu);
  out[2] = (unsigned char)((value >> 16u) & 0xffu);
  out[3] = (unsigned char)((value >> 24u) & 0xffu);
}

static void write_u64_le(unsigned char* out, uint64_t value) {
  for (unsigned int i = 0; i < 8u; ++i) out[i] = (unsigned char)((value >> (i * 8u)) & 0xffu);
}

static int hex_value_char(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
  if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
  return -1;
}

static int load_package_key(unsigned char key[32]) {
  const char* hex = getenv("VENOM_PACKAGE_KEY");
  if (!hex || !hex[0]) hex = getenv("VENOM_PACKAGE_KEY_HEX");
  if (!hex || strlen(hex) != 64u) return 0;
  for (size_t i = 0; i < 32u; ++i) {
    int hi = hex_value_char(hex[i * 2u]);
    int lo = hex_value_char(hex[i * 2u + 1u]);
    if (hi < 0 || lo < 0) return 0;
    key[i] = (unsigned char)((hi << 4) | lo);
  }
  return 1;
}

typedef int (*venom_sodium_init_fn)(void);
typedef int (*venom_sodium_decrypt_fn)(unsigned char*, unsigned long long*, unsigned char*, const unsigned char*, unsigned long long, const unsigned char*, unsigned long long, const unsigned char*, const unsigned char*);

typedef struct venom_sodium_api_t {
  int loaded;
  int ok;
#if defined(_WIN32)
  HMODULE handle;
#else
  void* handle;
#endif
  venom_sodium_init_fn sodium_init;
  venom_sodium_decrypt_fn decrypt;
} venom_sodium_api;

#if defined(_WIN32)
static void* sodium_sym(HMODULE handle, const char* name) { return (void*)GetProcAddress(handle, name); }
#else
static void* sodium_sym(void* handle, const char* name) { return dlsym(handle, name); }
#endif

static venom_sodium_api* sodium_api(void) {
  static venom_sodium_api api;
  if (api.loaded) return &api;
  api.loaded = 1;
#if defined(_WIN32)
  const char* names[] = {"libsodium.dll", "sodium.dll"};
  for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
    api.handle = LoadLibraryA(names[i]);
    if (api.handle) break;
  }
#else
  const char* names[] = {"libsodium.so", "libsodium.so.23", "libsodium.so.26"};
  for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
    api.handle = dlopen(names[i], RTLD_NOW | RTLD_LOCAL);
    if (api.handle) break;
  }
#endif
  if (!api.handle) return &api;
  {
    void* symbol = sodium_sym(api.handle, "sodium_init");
    memcpy(&api.sodium_init, &symbol, sizeof(api.sodium_init));
  }
  {
    void* symbol = sodium_sym(api.handle, "crypto_aead_xchacha20poly1305_ietf_decrypt");
    memcpy(&api.decrypt, &symbol, sizeof(api.decrypt));
  }
  api.ok = api.sodium_init && api.decrypt && api.sodium_init() >= 0;
  return &api;
}

static unsigned char* sodium_aad(uint32_t section_type, uint64_t name_digest, uint64_t plain_size, const unsigned char* name, uint32_t name_size, unsigned long long* out_size) {
  static const unsigned char magic[8] = {'V','S','O','D','I','U','M','1'};
  const uint64_t total = 8u + 4u + 4u + 8u + 8u + (uint64_t)name_size;
  if (total > (uint64_t)((size_t)-1)) return NULL;
  unsigned char* aad = (unsigned char*)malloc((size_t)total);
  if (!aad) return NULL;
  memcpy(aad, magic, 8u);
  write_u32_le(aad + 8u, 1u);
  write_u32_le(aad + 12u, section_type);
  write_u64_le(aad + 16u, plain_size);
  write_u64_le(aad + 24u, name_digest);
  if (name_size) memcpy(aad + 32u, name, name_size);
  *out_size = (unsigned long long)total;
  return aad;
}

static int open_libsodium_sealed_section_v1(const unsigned char* input,
                                            uint64_t input_size,
                                            uint32_t section_type,
                                            const unsigned char* name,
                                            uint32_t name_size,
                                            unsigned char** out_data,
                                            uint64_t* out_size) {
  static const unsigned char magic[8] = {'V','S','O','D','I','U','M','1'};
  const uint64_t header_size = 56u;
  const uint64_t tag_size = 16u;
  if (!input || !out_data || !out_size || input_size < header_size + tag_size) return VENOM_ERR_FORMAT;
  if (memcmp(input, magic, 8u) != 0) return VENOM_ERR_FORMAT;
  venom_sodium_api* api = sodium_api();
  if (!api->ok) return VENOM_ERR_SECTION;
  unsigned char key[32];
  if (!load_package_key(key)) return VENOM_ERR_SECTION;
  const uint32_t version = read_u32_le(input + 8u);
  const uint32_t sealed_type = read_u32_le(input + 12u);
  const uint64_t plain_size = read_u64_le(input + 16u);
  const uint64_t sealed_name_hash = read_u64_le(input + 24u);
  if (version != 1u) return VENOM_ERR_VERSION;
  if (sealed_type != section_type) return VENOM_ERR_SECTION;
  if (sealed_name_hash != fnv1a64_bytes(name, name_size)) return VENOM_ERR_SECTION;
  if (input_size - header_size != plain_size + tag_size) return VENOM_ERR_SECTION;
  if (plain_size > (uint64_t)((size_t)-1)) return VENOM_ERR_SECTION;

  unsigned long long aad_size = 0;
  unsigned char* aad = sodium_aad(section_type, sealed_name_hash, plain_size, name, name_size, &aad_size);
  if (!aad) return VENOM_ERR_SECTION;
  unsigned char* out = (unsigned char*)malloc((size_t)(plain_size ? plain_size : 1u));
  if (!out) { free(aad); return VENOM_ERR_SECTION; }
  unsigned long long decrypted_size = 0;
  int rc = api->decrypt(out, &decrypted_size, NULL, input + header_size, (unsigned long long)(input_size - header_size), aad, aad_size, input + 32u, key);
  free(aad);
  if (rc != 0 || decrypted_size != plain_size) {
    free(out);
    return VENOM_ERR_HASH;
  }
  *out_data = out;
  *out_size = plain_size;
  return VENOM_OK;
}

static int open_legacy_sealed_section_v1(const unsigned char* input,
                                         uint64_t input_size,
                                         uint32_t section_type,
                                         const unsigned char* name,
                                         uint32_t name_size,
                                         unsigned char** out_data,
                                         uint64_t* out_size) {
  static const unsigned char magic[8] = {'V','S','E','A','L','0','0','1'};
  if (!input || !out_data || !out_size || input_size < 40u) return VENOM_ERR_FORMAT;
  if (memcmp(input, magic, sizeof(magic)) != 0) return VENOM_ERR_FORMAT;
  const uint32_t version = read_u32_le(input + 8u);
  const uint32_t sealed_type = read_u32_le(input + 12u);
  const uint64_t plain_size = read_u64_le(input + 16u);
  const uint64_t plain_hash = read_u64_le(input + 24u);
  const uint64_t sealed_name_hash = read_u64_le(input + 32u);
  if (version != 1u) return VENOM_ERR_VERSION;
  if (sealed_type != section_type) return VENOM_ERR_SECTION;
  if (sealed_name_hash != fnv1a64_bytes(name, name_size)) return VENOM_ERR_SECTION;
  if (plain_size != input_size - 40u) return VENOM_ERR_SECTION;
  if (plain_size > (uint64_t)((size_t)-1)) return VENOM_ERR_SECTION;

  unsigned char* out = (unsigned char*)malloc((size_t)(plain_size ? plain_size : 1u));
  if (!out) return VENOM_ERR_SECTION;
  uint64_t state = seal_derive_seed(section_type, sealed_name_hash, plain_size, plain_hash);
  uint64_t word = 0;
  for (uint64_t i = 0; i < plain_size; ++i) {
    if ((i & 7u) == 0u) word = seal_next_word(&state);
    out[i] = (unsigned char)(input[40u + i] ^ (unsigned char)((word >> ((i & 7u) * 8u)) & 0xffu));
  }
  if (fnv1a64_bytes(out, plain_size) != plain_hash) {
    free(out);
    return VENOM_ERR_HASH;
  }
  *out_data = out;
  *out_size = plain_size;
  return VENOM_OK;
}

static int open_sealed_section_v1(const unsigned char* input,
                                  uint64_t input_size,
                                  uint32_t section_type,
                                  const unsigned char* name,
                                  uint32_t name_size,
                                  unsigned char** out_data,
                                  uint64_t* out_size) {
  static const unsigned char sodium_magic[8] = {'V','S','O','D','I','U','M','1'};
  static const unsigned char legacy_magic[8] = {'V','S','E','A','L','0','0','1'};
  static const unsigned char magic[8] = {'V','A','E','A','D','0','0','1'};
  if (!input || !out_data || !out_size || input_size < 8u) return VENOM_ERR_FORMAT;
  if (memcmp(input, sodium_magic, sizeof(sodium_magic)) == 0) {
    return open_libsodium_sealed_section_v1(input, input_size, section_type, name, name_size, out_data, out_size);
  }
  if (memcmp(input, legacy_magic, sizeof(legacy_magic)) == 0) {
    return open_legacy_sealed_section_v1(input, input_size, section_type, name, name_size, out_data, out_size);
  }
  if (input_size < 64u || memcmp(input, magic, sizeof(magic)) != 0) return VENOM_ERR_FORMAT;
  const uint32_t version = read_u32_le(input + 8u);
  const uint32_t sealed_type = read_u32_le(input + 12u);
  const uint64_t plain_size = read_u64_le(input + 16u);
  const uint64_t sealed_name_hash = read_u64_le(input + 24u);
  const uint64_t nonce_a = read_u64_le(input + 32u);
  const uint64_t nonce_b = read_u64_le(input + 40u);
  const uint64_t tag_a = read_u64_le(input + 48u);
  const uint64_t tag_b = read_u64_le(input + 56u);
  if (version != 1u) return VENOM_ERR_VERSION;
  if (sealed_type != section_type) return VENOM_ERR_SECTION;
  if (sealed_name_hash != fnv1a64_bytes(name, name_size)) return VENOM_ERR_SECTION;
  if (plain_size != input_size - 64u) return VENOM_ERR_SECTION;
  if (plain_size > (uint64_t)((size_t)-1)) return VENOM_ERR_SECTION;

  uint64_t expected_a = 0;
  uint64_t expected_b = 0;
  aead_tag(section_type, sealed_name_hash, plain_size, nonce_a, nonce_b, input + 64u, plain_size, &expected_a, &expected_b);
  if (expected_a != tag_a || expected_b != tag_b) return VENOM_ERR_HASH;

  unsigned char* out = (unsigned char*)malloc((size_t)(plain_size ? plain_size : 1u));
  if (!out) return VENOM_ERR_SECTION;
  uint64_t state = aead_stream_seed(section_type, sealed_name_hash, plain_size, nonce_a, nonce_b);
  uint64_t word = 0;
  for (uint64_t i = 0; i < plain_size; ++i) {
    if ((i & 7u) == 0u) word = seal_next_word(&state);
    out[i] = (unsigned char)(input[64u + i] ^ (unsigned char)((word >> ((i & 7u) * 8u)) & 0xffu));
  }
  *out_data = out;
  *out_size = plain_size;
  return VENOM_OK;
}


static int range_ok(uint64_t offset, uint64_t size, uint64_t file_size) {
  return offset <= file_size && size <= file_size && offset + size >= offset && offset + size <= file_size;
}

static int section_type_known(uint32_t value) {
  return value >= VENOM_SECTION_MANIFEST && value <= VENOM_SECTION_PACKAGE_FEATURES;
}

static uint64_t alias_update(uint64_t h, const unsigned char* bytes, uint64_t size) {
  for (uint64_t i = 0; i < size; ++i) {
    h ^= (uint64_t)bytes[i];
    h *= 1099511628211ULL;
  }
  return h;
}

static char alias_hex(unsigned value) {
  return (char)(value < 10u ? ('0' + value) : ('a' + (value - 10u)));
}

static int protected_alias_equals(uint32_t type, const unsigned char* stored, uint64_t stored_size, const char* expected) {
  if (type == VENOM_SECTION_ASSET || !stored || !expected || stored_size != 18u) return 0;
  uint64_t h = 1469598103934665603ULL;
  for (uint32_t i = 0; i < 4u; ++i) {
    const unsigned char b = (unsigned char)((type >> (i * 8u)) & 0xffu);
    h = alias_update(h, &b, 1u);
  }
  h = alias_update(h, (const unsigned char*)expected, (uint64_t)strlen(expected));
  char alias[18];
  alias[0] = 's';
  alias[1] = '.';
  for (int i = 17; i >= 2; --i) {
    alias[i] = alias_hex((unsigned)(h & 0x0fULL));
    h >>= 4u;
  }
  return memcmp(stored, alias, sizeof(alias)) == 0;
}

static int name_equals(const struct VenomSection* section, const char* expected) {
  if (!section || !expected) return 0;
  const size_t len = strlen(expected);
  if (section->name_size == len && memcmp(section->name, expected, len) == 0) return 1;
  return protected_alias_equals(section->type, section->name, section->name_size, expected);
}

static const struct VenomSection* find_section(const struct VenomPackageImage* image, uint32_t type, const char* name) {
  if (!image) return NULL;
  for (uint32_t i = 0; i < image->section_count; ++i) {
    const struct VenomSection* section = &image->sections[i];
    if (section->type == type && (!name || name_equals(section, name))) {
      return section;
    }
  }
  return NULL;
}

static int decompress_rle_v1(const unsigned char* input, uint64_t input_size, uint64_t expected_size, unsigned char** out_data) {
  static const unsigned char magic[8] = {'V','C','L','Z','0','0','0','8'};
  if (!input || !out_data || input_size < 20u) return VENOM_ERR_COMPRESS;
  if (memcmp(input, magic, 8u) != 0) return VENOM_ERR_COMPRESS;
  if (read_u32_le(input + 8u) != 1u) return VENOM_ERR_COMPRESS;
  if (read_u64_le(input + 12u) != expected_size) return VENOM_ERR_COMPRESS;
  if (expected_size > (uint64_t)((size_t)-1)) return VENOM_ERR_COMPRESS;

  unsigned char* out = (unsigned char*)malloc((size_t)(expected_size ? expected_size : 1u));
  if (!out) return VENOM_ERR_COMPRESS;
  uint64_t ip = 20u;
  uint64_t op = 0u;
  while (ip < input_size) {
    const unsigned char token = input[ip++];
    if ((token & 0x80u) != 0u) {
      const uint64_t length = (uint64_t)(token & 0x7fu) + 4u;
      if (ip + 2u > input_size) { free(out); return VENOM_ERR_COMPRESS; }
      const uint64_t distance = (uint64_t)input[ip] | ((uint64_t)input[ip + 1u] << 8u);
      ip += 2u;
      if (distance == 0u || distance > op || op + length > expected_size) { free(out); return VENOM_ERR_COMPRESS; }
      const uint64_t src = op - distance;
      for (uint64_t j = 0; j < length; ++j) {
        out[op++] = out[src + j];
      }
    } else {
      const uint64_t literal_len = (uint64_t)token + 1u;
      if (ip + literal_len > input_size || op + literal_len > expected_size) { free(out); return VENOM_ERR_COMPRESS; }
      memcpy(out + op, input + ip, (size_t)literal_len);
      op += literal_len;
      ip += literal_len;
    }
  }
  if (op != expected_size) { free(out); return VENOM_ERR_COMPRESS; }
  *out_data = out;
  return VENOM_OK;
}

static void free_image(struct VenomPackageImage* image) {
  if (!image || !image->sections) return;
  for (uint32_t i = 0; i < image->section_count; ++i) {
    free(image->sections[i].decoded.data);
  }
  free(image->sections);
  image->sections = NULL;
}

static int parse_package_image(const unsigned char* bytes, uint64_t size, struct VenomPackageImage* image) {
  static const unsigned char magic[8] = {'V','E','N','O','M','V','B','C'};
  const uint32_t known_flags = VENOM_PACKAGE_FLAG_PROTECT | VENOM_PACKAGE_FLAG_RELEASE |
    VENOM_PACKAGE_FLAG_POLYMORPHIC | VENOM_PACKAGE_FLAG_COMPRESSED |
    VENOM_PACKAGE_FLAG_CRYPTO_READY | VENOM_PACKAGE_FLAG_INTEGRITY_METADATA |
    VENOM_PACKAGE_FLAG_AEAD_READY | VENOM_PACKAGE_FLAG_RUNTIME_HARDENED |
    VENOM_PACKAGE_FLAG_WASM_RUNTIME | VENOM_PACKAGE_FLAG_HOST_BRIDGE |
    VENOM_PACKAGE_FLAG_BINARY_DOM_OPS | VENOM_PACKAGE_FLAG_FETCH_BRIDGE |
    VENOM_PACKAGE_FLAG_ASYNC_HOST_QUEUE | VENOM_PACKAGE_FLAG_TIMER_BRIDGE |
    VENOM_PACKAGE_FLAG_EVENT_QUEUE | VENOM_PACKAGE_FLAG_QUICKJS_BRIDGE |
    VENOM_PACKAGE_FLAG_SCRIPT_ISOLATION | VENOM_PACKAGE_FLAG_SCRIPT_POLICY | VENOM_PACKAGE_FLAG_QUICKJS_CHUNKS |
    VENOM_PACKAGE_FLAG_QUICKJS_ENGINE | VENOM_PACKAGE_FLAG_SCRIPT_ENGINE_FALLBACK |
    VENOM_PACKAGE_FLAG_QUICKJS_ENGINE_MODULE | VENOM_PACKAGE_FLAG_QUICKJS_CONTEXT_LIFECYCLE |
    VENOM_PACKAGE_FLAG_HOST_CAPABILITIES | VENOM_PACKAGE_FLAG_QUICKJS_ADAPTER_DIAGNOSTICS |
    VENOM_PACKAGE_FLAG_QUICKJS_WASM_RUNTIME | VENOM_PACKAGE_FLAG_QUICKJS_SOURCE_TRANSFER |
    VENOM_PACKAGE_FLAG_QUICKJS_CONSOLE_BRIDGE | VENOM_PACKAGE_FLAG_QUICKJS_EXECUTION_RECORDS |
    VENOM_PACKAGE_FLAG_QUICKJS_RESULT_BRIDGE | VENOM_PACKAGE_FLAG_QUICKJS_FALLBACK_POLICY |
    VENOM_PACKAGE_FLAG_QUICKJS_ENGINE_BACKEND;
  const uint32_t known_section_flags = VENOM_SECTION_COMPRESSED | VENOM_SECTION_ENCRYPTED;

  if (!bytes || !image) return VENOM_ERR_ARGUMENT;
  memset(image, 0, sizeof(*image));
  image->bytes = bytes;
  image->size = size;
  if (size < VENOM_HEADER_SIZE) return VENOM_ERR_TRUNCATED;
  if (memcmp(bytes, magic, sizeof(magic)) != 0) return VENOM_ERR_MAGIC;

  image->version = read_u32_le(bytes + 8u);
  image->abi = read_u32_le(bytes + 12u);
  image->flags = read_u32_le(bytes + 16u);
  image->section_count = read_u32_le(bytes + 20u);
  image->section_table_offset = read_u64_le(bytes + 24u);
  image->section_table_size = read_u64_le(bytes + 32u);
  image->name_table_offset = read_u64_le(bytes + 40u);
  image->name_table_size = read_u64_le(bytes + 48u);
  image->payload_offset = read_u64_le(bytes + 56u);
  image->payload_size = read_u64_le(bytes + 64u);
  image->package_hash = read_u64_le(bytes + VENOM_PACKAGE_HASH_OFFSET);

  if (image->version != VENOM_PACKAGE_VERSION) return VENOM_ERR_VERSION;
  if (image->abi != VENOM_RUNTIME_ABI) return VENOM_ERR_ABI;
  if ((image->flags & ~known_flags) != 0u) return VENOM_ERR_FLAGS;
  if (image->section_table_offset != VENOM_HEADER_SIZE) return VENOM_ERR_RANGE;
  if (image->section_table_size != (uint64_t)image->section_count * VENOM_SECTION_ENTRY_SIZE) return VENOM_ERR_RANGE;
  if (!range_ok(image->section_table_offset, image->section_table_size, size)) return VENOM_ERR_RANGE;
  if (!range_ok(image->name_table_offset, image->name_table_size, size)) return VENOM_ERR_RANGE;
  if (!range_ok(image->payload_offset, image->payload_size, size)) return VENOM_ERR_RANGE;
  if (fnv1a64_package(bytes, size) != image->package_hash) return VENOM_ERR_HASH;

  if (image->section_count > 4096u) return VENOM_ERR_SECTION;
  image->sections = (struct VenomSection*)calloc(image->section_count ? image->section_count : 1u, sizeof(struct VenomSection));
  if (!image->sections) return VENOM_ERR_SECTION;

  for (uint32_t i = 0; i < image->section_count; ++i) {
    const uint64_t entry = image->section_table_offset + (uint64_t)i * VENOM_SECTION_ENTRY_SIZE;
    struct VenomSection* section = &image->sections[i];
    const uint32_t name_offset = read_u32_le(bytes + entry + 8u);
    section->type = read_u32_le(bytes + entry + 0u);
    section->flags = read_u32_le(bytes + entry + 4u);
    section->name_size = read_u32_le(bytes + entry + 12u);
    section->offset = read_u64_le(bytes + entry + 16u);
    section->stored_size = read_u64_le(bytes + entry + 24u);
    section->raw_size = read_u64_le(bytes + entry + 32u);
    section->hash = read_u64_le(bytes + entry + 40u);

    if (!section_type_known(section->type)) { free_image(image); return VENOM_ERR_SECTION; }
    if ((section->flags & ~known_section_flags) != 0u) { free_image(image); return VENOM_ERR_FLAGS; }
    if (!range_ok(image->name_table_offset + name_offset, section->name_size, size)) { free_image(image); return VENOM_ERR_RANGE; }
    if (!range_ok(section->offset, section->stored_size, size)) { free_image(image); return VENOM_ERR_RANGE; }
    if (section->offset < image->payload_offset || section->offset + section->stored_size > image->payload_offset + image->payload_size) { free_image(image); return VENOM_ERR_RANGE; }

    section->name = bytes + image->name_table_offset + name_offset;
    const unsigned char* stored = bytes + section->offset;
    if (fnv1a64_bytes(stored, section->stored_size) != section->hash) { free_image(image); return VENOM_ERR_HASH; }

    const unsigned char* staged = stored;
    uint64_t staged_size = section->stored_size;
    unsigned char* opened = NULL;
    if ((section->flags & VENOM_SECTION_ENCRYPTED) != 0u) {
      int rc = open_sealed_section_v1(stored, section->stored_size, section->type, section->name, section->name_size, &opened, &staged_size);
      if (rc != VENOM_OK) { free_image(image); return rc; }
      staged = opened;
    }

    if ((section->flags & VENOM_SECTION_COMPRESSED) != 0u) {
      if ((section->flags & VENOM_SECTION_ENCRYPTED) == 0u && section->raw_size < staged_size) { free(opened); free_image(image); return VENOM_ERR_COMPRESS; }
      int rc = decompress_rle_v1(staged, staged_size, section->raw_size, &section->decoded.data);
      free(opened);
      if (rc != VENOM_OK) { free_image(image); return rc; }
      section->decoded.size = section->raw_size;
    } else {
      if (section->raw_size != staged_size) { free(opened); free_image(image); return VENOM_ERR_SECTION; }
      section->decoded.data = (unsigned char*)malloc((size_t)(staged_size ? staged_size : 1u));
      if (!section->decoded.data) { free(opened); free_image(image); return VENOM_ERR_SECTION; }
      memcpy(section->decoded.data, staged, (size_t)staged_size);
      section->decoded.size = staged_size;
      free(opened);
    }
  }

  if ((image->flags & VENOM_PACKAGE_FLAG_RELEASE) != 0u) {
    if ((image->flags & VENOM_PACKAGE_FLAG_POLYMORPHIC) == 0u) return VENOM_ERR_FLAGS;
    if ((image->flags & VENOM_PACKAGE_FLAG_INTEGRITY_METADATA) == 0u) return VENOM_ERR_FLAGS;
    if ((image->flags & VENOM_PACKAGE_FLAG_RUNTIME_HARDENED) == 0u) return VENOM_ERR_FLAGS;
    if (!find_section(image, VENOM_SECTION_PACKAGE_FEATURES, "package-features.vfeat")) return VENOM_ERR_MISSING;
  }

  return VENOM_OK;
}

static int require_ascii(const struct VenomSection* section, const char* magic) {
  const size_t len = strlen(magic);
  return section && section->decoded.size >= len && memcmp(section->decoded.data, magic, len) == 0;
}

static uint32_t string_stream_next(uint32_t* state) {
  *state ^= *state << 13u;
  *state ^= *state >> 17u;
  *state ^= *state << 5u;
  return *state;
}

static int parse_string_table(const struct VenomSection* section, struct VenomStringTable* table) {
  if (!require_ascii(section, "VSTR0011") || !table) return VENOM_ERR_FORMAT;
  const unsigned char* data = section->decoded.data;
  const uint64_t size = section->decoded.size;
  if (size < 20u) return VENOM_ERR_TRUNCATED;
  if (read_u32_le(data + 8u) != 2u) return VENOM_ERR_VERSION;
  const uint32_t count = read_u32_le(data + 12u);
  const uint32_t table_seed = read_u32_le(data + 16u);
  const uint64_t entry_base = 20u;
  const uint64_t data_base = entry_base + (uint64_t)count * 12u;
  if (data_base > size) return VENOM_ERR_RANGE;
  table->strings = (struct VenomStringRef*)calloc(count ? count : 1u, sizeof(struct VenomStringRef));
  table->decoded_data = (unsigned char*)calloc(size - data_base ? (size_t)(size - data_base) : 1u, 1u);
  if (!table->strings || !table->decoded_data) return VENOM_ERR_FORMAT;
  table->count = count;
  for (uint32_t i = 0; i < count; ++i) {
    const uint64_t entry = entry_base + (uint64_t)i * 12u;
    const uint32_t off = read_u32_le(data + entry);
    const uint32_t len = read_u32_le(data + entry + 4u);
    const uint32_t nonce = read_u32_le(data + entry + 8u);
    if (data_base + off + len < data_base + off || data_base + off + len > size) return VENOM_ERR_RANGE;
    uint32_t state = table_seed ^ nonce ^ (len * 0x27D4EB2Du);
    for (uint32_t j = 0; j < len; ++j) table->decoded_data[off + j] = data[data_base + off + j] ^ (unsigned char)(string_stream_next(&state) & 0xffu);
    table->strings[i].data = table->decoded_data + off;
    table->strings[i].size = len;
  }
  return VENOM_OK;
}

static int parse_route_table(const struct VenomSection* section, struct VenomRouteTable* table) {
  if (!require_ascii(section, "VRTE0003") || !table) return VENOM_ERR_FORMAT;
  const unsigned char* data = section->decoded.data;
  const uint64_t size = section->decoded.size;
  if (size < 16u) return VENOM_ERR_TRUNCATED;
  if (read_u32_le(data + 8u) != 2u) return VENOM_ERR_VERSION;
  const uint32_t count = read_u32_le(data + 12u);
  const uint64_t needed = 16u + (uint64_t)count * 24u;
  if (needed > size) return VENOM_ERR_RANGE;
  table->routes = (struct VenomRouteEntry*)calloc(count ? count : 1u, sizeof(struct VenomRouteEntry));
  if (!table->routes) return VENOM_ERR_FORMAT;
  table->count = count;
  for (uint32_t i = 0; i < count; ++i) {
    const unsigned char* p = data + 16u + (uint64_t)i * 24u;
    table->routes[i].route_id = read_u32_le(p + 0u);
    table->routes[i].source_id = read_u32_le(p + 4u);
    table->routes[i].bytecode_offset = read_u32_le(p + 8u);
    table->routes[i].bytecode_size = read_u32_le(p + 12u);
    table->routes[i].instruction_count = read_u32_le(p + 16u);
    table->routes[i].flags = read_u32_le(p + 20u);
  }
  return VENOM_OK;
}

static int parse_vm_bytecode(const struct VenomSection* section, struct VenomVmBytecode* vm) {
  if (!require_ascii(section, "VBCODE03") || !vm) return VENOM_ERR_FORMAT;
  const unsigned char* data = section->decoded.data;
  const uint64_t size = section->decoded.size;
  if (size < 24u) return VENOM_ERR_TRUNCATED;
  if (read_u32_le(data + 8u) != 1u) return VENOM_ERR_VERSION;
  vm->instruction_size = read_u32_le(data + 12u);
  vm->instruction_count = read_u32_le(data + 16u);
  vm->flags = read_u32_le(data + 20u);
  if (vm->instruction_size != 16u) return VENOM_ERR_FORMAT;
  if (size - 24u > (uint64_t)UINT32_MAX) return VENOM_ERR_RANGE;
  vm->data = data + 24u;
  vm->data_size = (uint32_t)(size - 24u);
  return VENOM_OK;
}

static void poly_init_default(struct VenomPolyMap* poly) {
  memset(poly, 0, sizeof(*poly));
  poly->word_layout[0] = VENOM_WORD_OPCODE;
  poly->word_layout[1] = VENOM_WORD_A;
  poly->word_layout[2] = VENOM_WORD_B;
  poly->word_layout[3] = VENOM_WORD_C;
  for (uint32_t i = 0; i <= VENOM_LOGICAL_HALT; ++i) {
    poly->physical_to_logical[i] = i;
    poly->has_physical[i / 32u] |= 1u << (i % 32u);
  }
}

static int parse_poly_section(const struct VenomSection* section, struct VenomPolyMap* poly) {
  if (!section || !poly || !require_ascii(section, "VPOL0010")) return VENOM_ERR_FORMAT;
  const unsigned char* data = section->decoded.data;
  const uint64_t size = section->decoded.size;
  if (size < 72u) return VENOM_ERR_TRUNCATED;
  if (read_u32_le(data + 8u) != 2u) return VENOM_ERR_VERSION;
  poly_init_default(poly);
  poly->opcode_mask = read_u32_le(data + 20u);
  poly->operand_masks[0] = read_u32_le(data + 24u);
  poly->operand_masks[1] = read_u32_le(data + 28u);
  poly->operand_masks[2] = read_u32_le(data + 32u);
  for (uint32_t i = 0; i < 4u; ++i) {
    const uint32_t word = data[36u + i];
    if (word > VENOM_WORD_C) return VENOM_ERR_FORMAT;
    poly->word_layout[i] = word;
  }
  const uint32_t count = read_u32_le(data + 48u);
  if (72u + (uint64_t)count * 4u > size) return VENOM_ERR_RANGE;
  for (uint32_t i = 0; i < count; ++i) {
    const unsigned char* entry = data + 72u + (uint64_t)i * 4u;
    const uint32_t logical = (uint32_t)entry[0] | ((uint32_t)entry[1] << 8u);
    const uint32_t physical = (uint32_t)entry[2] | ((uint32_t)entry[3] << 8u);
    poly->physical_to_logical[physical] = logical;
    poly->has_physical[physical / 32u] |= 1u << (physical % 32u);
  }
  return count ? VENOM_OK : VENOM_ERR_FORMAT;
}

static int string_equals(const struct VenomStringTable* table, uint32_t id, const char* value) {
  const size_t len = strlen(value);
  if (!table || id >= table->count) return 0;
  return table->strings[id].size == len && memcmp(table->strings[id].data, value, len) == 0;
}

static const struct VenomRouteEntry* select_route(const struct VenomRouteTable* routes, const struct VenomStringTable* strings, const char* route) {
  const char* wanted = (route && route[0]) ? route : "/";
  const struct VenomRouteEntry* first = routes && routes->count ? &routes->routes[0] : NULL;
  const struct VenomRouteEntry* root = NULL;
  if (!routes || !strings) return NULL;
  for (uint32_t i = 0; i < routes->count; ++i) {
    const struct VenomRouteEntry* candidate = &routes->routes[i];
    if (string_equals(strings, candidate->route_id, wanted)) return candidate;
    if (string_equals(strings, candidate->route_id, "/")) root = candidate;
  }
  return root ? root : first;
}

static void copy_string_to(char* out, unsigned int cap, const unsigned char* data, uint32_t size) {
  if (!out || cap == 0u) return;
  const uint32_t n = size + 1u < cap ? size : cap - 1u;
  if (n) memcpy(out, data, n);
  out[n] = 0;
}

static void append_text(char* out, unsigned int cap, const unsigned char* data, uint32_t size) {
  if (!out || cap == 0u) return;
  const size_t current = strlen(out);
  if (current + 1u >= cap) return;
  const size_t room = cap - current - 1u;
  const size_t n = size < room ? size : room;
  if (n) memcpy(out + current, data, n);
  out[current + n] = 0;
}

static int decode_instruction(const struct VenomVmBytecode* vm, uint32_t pc, const struct VenomPolyMap* poly, uint32_t* logical, uint32_t* a, uint32_t* b, uint32_t* c) {
  if (!vm || !poly || pc + 16u > vm->data_size) return VENOM_ERR_VM;
  uint32_t words[4];
  words[0] = read_u32_le(vm->data + pc + 0u);
  words[1] = read_u32_le(vm->data + pc + 4u);
  words[2] = read_u32_le(vm->data + pc + 8u);
  words[3] = read_u32_le(vm->data + pc + 12u);
  uint32_t physical = 0;
  uint32_t aa = 0, bb = 0, cc = 0;
  for (uint32_t i = 0; i < 4u; ++i) {
    const uint32_t value = words[i];
    switch (poly->word_layout[i]) {
      case VENOM_WORD_OPCODE: physical = value ^ poly->opcode_mask; break;
      case VENOM_WORD_A: aa = value ^ poly->operand_masks[0]; break;
      case VENOM_WORD_B: bb = value ^ poly->operand_masks[1]; break;
      case VENOM_WORD_C: cc = value ^ poly->operand_masks[2]; break;
      default: return VENOM_ERR_VM;
    }
  }
  if (physical >= 65536u || (poly->has_physical[physical / 32u] & (1u << (physical % 32u))) == 0u) return VENOM_ERR_VM;
  *logical = poly->physical_to_logical[physical];
  *a = aa; *b = bb; *c = cc;
  return VENOM_OK;
}

static int execute_route_text(const struct VenomRouteEntry* route, const struct VenomVmBytecode* vm, const struct VenomStringTable* strings, const struct VenomPolyMap* poly, VenomRuntimeReport* report) {
  if (!route || !vm || !strings || !poly || !report) return VENOM_ERR_ARGUMENT;
  if (route->bytecode_offset + route->bytecode_size < route->bytecode_offset || route->bytecode_offset + route->bytecode_size > vm->data_size) return VENOM_ERR_RANGE;
  uint32_t pc = route->bytecode_offset;
  const uint32_t end = route->bytecode_offset + route->bytecode_size;
  unsigned int stack_depth = 1u;
  int saw_halt = 0;
  while (pc < end) {
    uint32_t op = 0, a = 0, b = 0, c = 0;
    int rc = decode_instruction(vm, pc, poly, &op, &a, &b, &c);
    (void)b; (void)c;
    if (rc != VENOM_OK) return rc;
    pc += vm->instruction_size;
    report->executed_instruction_count++;
    switch (op) {
      case VENOM_LOGICAL_NOP:
      case VENOM_LOGICAL_LOAD_CONST:
      case VENOM_LOGICAL_CALL_QUICKJS:
      case VENOM_LOGICAL_CREATE_ELEMENT:
      case VENOM_LOGICAL_SET_ATTRIBUTE:
      case VENOM_LOGICAL_APPEND_CHILD:
        break;
      case VENOM_LOGICAL_ENTER_ELEMENT:
        stack_depth++;
        break;
      case VENOM_LOGICAL_LEAVE_ELEMENT:
        if (stack_depth <= 1u) return VENOM_ERR_VM;
        stack_depth--;
        break;
      case VENOM_LOGICAL_SET_TEXT:
        if (a >= strings->count) return VENOM_ERR_VM;
        append_text(report->rendered_text, VENOM_RUNTIME_TEXT_CAP, strings->strings[a].data, strings->strings[a].size);
        break;
      case VENOM_LOGICAL_HALT:
        saw_halt = 1;
        pc = end;
        break;
      default:
        return VENOM_ERR_VM;
    }
  }
  return saw_halt ? VENOM_OK : VENOM_ERR_VM;
}

static unsigned int count_script_chunks(const struct VenomSection* section) {
  if (!require_ascii(section, "VJSB0006") || section->decoded.size < 16u) return 0u;
  return read_u32_le(section->decoded.data + 12u);
}

static unsigned int count_asset_manifest(const struct VenomSection* section) {
  if (!section || section->decoded.size == 0u) return 0u;
  unsigned int count = 0u;
  const unsigned char* data = section->decoded.data;
  for (uint64_t i = 0; i < section->decoded.size; ++i) {
    if (data[i] == '\n') count++;
  }
  return count > 4u ? count - 4u : 0u;
}

unsigned int venom_runtime_abi(void) {
  return VENOM_RUNTIME_ABI;
}

unsigned int venom_package_version(void) {
  return VENOM_PACKAGE_VERSION;
}

int venom_package_validate(const unsigned char* bytes, unsigned long size) {
  struct VenomPackageImage image;
  int rc = parse_package_image(bytes, (uint64_t)size, &image);
  if (rc == VENOM_OK) free_image(&image);
  return rc;
}

int venom_runtime_analyze(const unsigned char* bytes, unsigned long size, const char* route_name, VenomRuntimeReport* report) {
  if (!report) return VENOM_ERR_ARGUMENT;
  memset(report, 0, sizeof(*report));

  struct VenomPackageImage image;
  int rc = parse_package_image(bytes, (uint64_t)size, &image);
  if (rc != VENOM_OK) return rc;

  report->package_version = image.version;
  report->runtime_abi = image.abi;
  report->package_flags = image.flags;
  report->section_count = image.section_count;

  const struct VenomSection* strings_section = find_section(&image, VENOM_SECTION_STRINGS, "strings.vstr");
  const struct VenomSection* routes_section = find_section(&image, VENOM_SECTION_ROUTES, "routes.vbrt");
  const struct VenomSection* bytecode_section = find_section(&image, VENOM_SECTION_VM_BYTECODE, "route-bytecode.vmbc");
  const struct VenomSection* poly_section = find_section(&image, VENOM_SECTION_INTEGRITY, "vm-polymorph.vpol");
  const struct VenomSection* scripts_section = find_section(&image, VENOM_SECTION_JAVASCRIPT, "scripts.vjsb");
  const struct VenomSection* asset_section = find_section(&image, VENOM_SECTION_ASSET_MANIFEST, "asset-manifest.txt");
  if (!strings_section || !routes_section || !bytecode_section || !poly_section) { free_image(&image); return VENOM_ERR_MISSING; }

  struct VenomStringTable strings;
  struct VenomRouteTable routes;
  struct VenomVmBytecode vm;
  struct VenomPolyMap poly;
  memset(&strings, 0, sizeof(strings));
  memset(&routes, 0, sizeof(routes));
  memset(&vm, 0, sizeof(vm));
  memset(&poly, 0, sizeof(poly));

  rc = parse_string_table(strings_section, &strings);
  if (rc == VENOM_OK) rc = parse_route_table(routes_section, &routes);
  if (rc == VENOM_OK) rc = parse_vm_bytecode(bytecode_section, &vm);
  if (rc == VENOM_OK) rc = parse_poly_section(poly_section, &poly);
  if (rc != VENOM_OK) {
    free(strings.decoded_data); free(strings.strings);
    free(routes.routes);
    free_image(&image);
    return rc;
  }

  report->string_count = strings.count;
  report->route_count = routes.count;
  report->total_instruction_count = vm.instruction_count;
  report->script_chunk_count = count_script_chunks(scripts_section);
  report->asset_count = count_asset_manifest(asset_section);

  const struct VenomRouteEntry* selected = select_route(&routes, &strings, route_name);
  if (!selected || selected->route_id >= strings.count) {
    rc = VENOM_ERR_MISSING;
  } else {
    copy_string_to(report->selected_route, VENOM_RUNTIME_ROUTE_CAP, strings.strings[selected->route_id].data, strings.strings[selected->route_id].size);
    rc = execute_route_text(selected, &vm, &strings, &poly, report);
  }

  free(strings.decoded_data); free(strings.strings);
  free(routes.routes);
  free_image(&image);
  return rc;
}
