#include <stdint.h>

void* memset(void* dst, int value, unsigned long count) {
  unsigned char* p = (unsigned char*)dst;
  for (unsigned long i = 0; i < count; ++i) p[i] = (unsigned char)value;
  return dst;
}

void* memcpy(void* dst, const void* src, unsigned long count) {
  unsigned char* d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;
  for (unsigned long i = 0; i < count; ++i) d[i] = s[i];
  return dst;
}

#define VENOM_RUNTIME_ABI 1u
#define VENOM_PACKAGE_VERSION 40u
#define VENOM_HEADER_SIZE 80u
#define VENOM_SECTION_ENTRY_SIZE 48u
#define VENOM_PACKAGE_HASH_OFFSET 72u

#define VENOM_PACKAGE_FLAG_RELEASE 2u
#define VENOM_PACKAGE_FLAG_POLYMORPHIC 4u
#define VENOM_PACKAGE_FLAG_INTEGRITY_METADATA 32u
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

#define VENOM_SECTION_ROUTES 2u
#define VENOM_SECTION_JAVASCRIPT 5u
#define VENOM_SECTION_VM_BYTECODE 7u
#define VENOM_SECTION_ASSET 8u
#define VENOM_SECTION_INTEGRITY 9u
#define VENOM_SECTION_STRINGS 10u
#define VENOM_SECTION_ASSET_MANIFEST 11u
#define VENOM_SECTION_HOST_BRIDGE 12u
#define VENOM_SECTION_FETCH_BRIDGE 13u
#define VENOM_SECTION_ASYNC_HOST_QUEUE 14u

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

#define WORD_OPCODE 0u
#define WORD_A 1u
#define WORD_B 2u
#define WORD_C 3u

#define ERR_OK 0
#define ERR_ARGUMENT 1
#define ERR_TRUNCATED 2
#define ERR_MAGIC 3
#define ERR_VERSION 4
#define ERR_ABI 5
#define ERR_FLAGS 6
#define ERR_RANGE 7
#define ERR_HASH 8
#define ERR_SECTION 9
#define ERR_COMPRESS 10
#define ERR_FORMAT 11
#define ERR_MISSING 12
#define ERR_VM 13
#define ERR_CAPACITY 14

#define PACKAGE_CAP (2u * 1024u * 1024u)
#define ROUTE_CAP 256u
#define RESULT_CAP 65536u
#define DOMOPS_CAP 262144u
#define DOMOP_BIN_CAP 262144u
#define STRINGS_CAP 131072u
#define ROUTES_CAP 32768u
#define BYTECODE_CAP 262144u
#define POLY_CAP 65536u
#define JS_CAP 131072u
#define ASSET_CAP 131072u
#define PACKAGE_META_CAP 262144u
#define ROUTE_RECORD_CAP 2048u
#define VENOM_MAX_SECTIONS 4096u
#define VENOM_SECTION_TYPE_MAX 88u

#define DOMOP_CREATE_ELEMENT 1u
#define DOMOP_ENTER_ELEMENT 2u
#define DOMOP_SET_ATTRIBUTE 3u
#define DOMOP_BIND_EVENT 4u
#define DOMOP_SET_TEXT 5u
#define DOMOP_LEAVE_ELEMENT 6u
#define DOMOP_APPEND_CHILD 7u

static unsigned char g_package[PACKAGE_CAP];
static unsigned char g_route[ROUTE_CAP];
static unsigned char g_result[RESULT_CAP];
static unsigned int g_result_size;
static unsigned char g_domops[DOMOPS_CAP];
static unsigned int g_domops_size;
static int g_domops_overflow;
static unsigned char g_domop_bin[DOMOP_BIN_CAP];
static unsigned int g_domop_bin_size;
static int g_domop_bin_overflow;
static unsigned char g_strings[STRINGS_CAP];
static unsigned char g_routes[ROUTES_CAP];
static unsigned char g_bytecode[BYTECODE_CAP];
static unsigned char g_poly_text[POLY_CAP];
static unsigned char g_js[JS_CAP];
static unsigned char g_asset[ASSET_CAP];
static unsigned char g_decode_stage[PACKAGE_CAP];
static unsigned char g_materialized_section[PACKAGE_CAP];
static uint32_t g_materialized_section_size = 0u;
static unsigned char g_package_meta[PACKAGE_META_CAP];
static uint32_t g_package_meta_size = 0u;
static unsigned char g_route_record[ROUTE_RECORD_CAP];
static uint32_t g_route_record_size = 0u;
static uint32_t g_package_loaded_size = 0u;
static uint32_t g_route_generation = 0u;

static void venom_wasm_secure_zero(void* ptr, uint32_t size) {
  volatile unsigned char* p = (volatile unsigned char*)ptr;
  while (size--) *p++ = 0u;
}

static void venom_wasm_release_materialized(void) {
  if (g_materialized_section_size) venom_wasm_secure_zero(g_materialized_section, g_materialized_section_size);
  g_materialized_section_size = 0u;
  ++g_route_generation;
}

static void venom_wasm_release_transient(void) {
  venom_wasm_release_materialized();
  if (g_route_record_size) venom_wasm_secure_zero(g_route_record, g_route_record_size);
  if (g_result_size) venom_wasm_secure_zero(g_result, g_result_size);
  if (g_domops_size) venom_wasm_secure_zero(g_domops, g_domops_size);
  if (g_domop_bin_size) venom_wasm_secure_zero(g_domop_bin, g_domop_bin_size);
  venom_wasm_secure_zero(g_route, ROUTE_CAP);
  venom_wasm_secure_zero(g_strings, STRINGS_CAP);
  venom_wasm_secure_zero(g_routes, ROUTES_CAP);
  venom_wasm_secure_zero(g_bytecode, BYTECODE_CAP);
  venom_wasm_secure_zero(g_poly_text, POLY_CAP);
  venom_wasm_secure_zero(g_js, JS_CAP);
  venom_wasm_secure_zero(g_asset, ASSET_CAP);
  venom_wasm_secure_zero(g_decode_stage, PACKAGE_CAP);
  g_route_record_size = 0u;
  g_result_size = 0u;
  g_domops_size = 0u;
  g_domops_overflow = 0;
  g_domop_bin_size = 0u;
  g_domop_bin_overflow = 0;
}
static int g_package_loaded = 0;
static uint32_t g_phys_to_logical[65536];
static uint32_t g_has_phys[65536 / 32];
static uint32_t g_dom_logical_to_physical[8] = {0u,1u,2u,3u,4u,5u,6u,7u};
static uint32_t g_dom_field_layout[3] = {0u,1u,2u};

static int mem_eq(const unsigned char* a, const char* b, uint32_t n) {
  for (uint32_t i = 0; i < n; ++i) if (a[i] != (unsigned char)b[i]) return 0;
  return 1;
}

static uint32_t str_len(const char* s) {
  uint32_t n = 0;
  while (s && s[n]) ++n;
  return n;
}

static char alias_hex(unsigned value) {
  return (char)(value < 10u ? ('0' + value) : ('a' + (value - 10u)));
}

static int protected_alias_eq_bytes(uint32_t type, const unsigned char* a, uint32_t a_len, const char* b) {
  if (type == VENOM_SECTION_ASSET || !a || !b || a_len != 18u) return 0;
  uint64_t h = 1469598103934665603ULL;
  const uint64_t prime = 1099511628211ULL;
  for (uint32_t i = 0; i < 4u; ++i) {
    h ^= (uint64_t)((type >> (i * 8u)) & 0xffu);
    h *= prime;
  }
  const uint32_t b_len = str_len(b);
  for (uint32_t i = 0; i < b_len; ++i) {
    h ^= (uint64_t)((const unsigned char*)b)[i];
    h *= prime;
  }
  char alias[18];
  alias[0] = 's';
  alias[1] = '.';
  for (int i = 17; i >= 2; --i) {
    alias[i] = alias_hex((unsigned)(h & 0x0fULL));
    h >>= 4u;
  }
  return mem_eq(a, alias, 18u);
}

static int name_eq_bytes(const unsigned char* a, uint32_t a_len, const char* b) {
  const uint32_t b_len = str_len(b);
  if (a_len != b_len) return 0;
  for (uint32_t i = 0; i < a_len; ++i) if (a[i] != (unsigned char)b[i]) return 0;
  return 1;
}

static int section_name_eq_bytes(uint32_t type, const unsigned char* a, uint32_t a_len, const char* b) {
  return name_eq_bytes(a, a_len, b) || protected_alias_eq_bytes(type, a, a_len, b);
}



static uint32_t normalize_route_bytes(const unsigned char* in, uint32_t n, unsigned char* out, uint32_t cap) {
  if (!out || cap < 2u) return 0u;
  uint32_t o=0u;
  if (!in || n==0u || in[0]!='/') out[o++]='/';
  for (uint32_t i=0u;i<n && o+1u<cap;++i) { unsigned char c=in[i]; if (c=='?' || c=='#') break; out[o++]=c; }
  while (o>1u && out[o-1u]=='/') --o;
  if (o==11u && mem_eq(out, "/index.html", 11u)) o=1u;
  else if (o==10u && mem_eq(out, "/index.htm", 10u)) o=1u;
  else if (o>5u && mem_eq(out+o-5u, ".html", 5u)) o-=5u;
  else if (o>4u && mem_eq(out+o-4u, ".htm", 4u)) o-=4u;
  if (o>6u && mem_eq(out+o-6u, "/index", 6u)) { o-=6u; if (!o) out[o++]='/'; }
  return o;
}

static uint32_t read_u32(const unsigned char* p) {
  return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8u) | ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static uint64_t read_u64(const unsigned char* p) {
  uint64_t v = 0;
  for (uint32_t i = 0; i < 8u; ++i) v |= ((uint64_t)p[i]) << (i * 8u);
  return v;
}

static void copy_bytes(unsigned char* dst, const unsigned char* src, uint32_t n) {
  for (uint32_t i = 0; i < n; ++i) dst[i] = src[i];
}

static uint64_t fnv_update(uint64_t hash, const unsigned char* bytes, uint32_t size) {
  const uint64_t prime = 1099511628211ULL;
  for (uint32_t i = 0; i < size; ++i) {
    hash ^= (uint64_t)bytes[i];
    hash *= prime;
  }
  return hash;
}

static uint64_t fnv_bytes(const unsigned char* bytes, uint32_t size) {
  return fnv_update(1469598103934665603ULL, bytes, size);
}

static uint64_t fnv_package(const unsigned char* bytes, uint32_t size) {
  uint64_t hash = 1469598103934665603ULL;
  if (size <= VENOM_PACKAGE_HASH_OFFSET) return fnv_bytes(bytes, size);
  hash = fnv_update(hash, bytes, VENOM_PACKAGE_HASH_OFFSET);
  static const unsigned char zeros[8] = {0,0,0,0,0,0,0,0};
  hash = fnv_update(hash, zeros, 8u);
  if (size > VENOM_PACKAGE_HASH_OFFSET + 8u) {
    hash = fnv_update(hash, bytes + VENOM_PACKAGE_HASH_OFFSET + 8u, size - VENOM_PACKAGE_HASH_OFFSET - 8u);
  }
  return hash;
}


static uint64_t fnv_update_u64(uint64_t hash, uint64_t value) {
  unsigned char bytes[8];
  for (uint32_t i = 0; i < 8u; ++i) bytes[i] = (unsigned char)((value >> (i * 8u)) & 0xffu);
  return fnv_update(hash, bytes, 8u);
}

static uint64_t seal_mix64(uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30u)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27u)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31u);
}

static uint64_t seal_derive_seed(uint32_t section_type, uint64_t name_digest, uint64_t plain_size, uint64_t plain_hash) {
  uint64_t h = 1469598103934665603ULL;
  h = fnv_update_u64(h, (uint64_t)section_type);
  h = fnv_update_u64(h, name_digest);
  h = fnv_update_u64(h, plain_size);
  h = fnv_update_u64(h, plain_hash);
  h ^= 0x7f4a7c159e3779b9ULL;
  h = seal_mix64(h ^ 0xd6e8feb86659fd93ULL);
  return h == 0u ? 0x7f4a7c159e3779b9ULL : h;
}

static uint64_t aead_stream_seed(uint32_t section_type, uint64_t name_digest, uint64_t plain_size, uint64_t nonce_a, uint64_t nonce_b) {
  uint64_t h = 1469598103934665603ULL ^ 0xa41bc927f35d69e1ULL;
  h = fnv_update_u64(h, (uint64_t)section_type);
  h = fnv_update_u64(h, name_digest);
  h = fnv_update_u64(h, plain_size);
  h = fnv_update_u64(h, nonce_a);
  h = fnv_update_u64(h, nonce_b);
  return seal_mix64(h ^ 0x6c8e9cf570932bd5ULL);
}

static void aead_tag(uint32_t section_type, uint64_t name_digest, uint64_t plain_size, uint64_t nonce_a, uint64_t nonce_b, const unsigned char* ciphertext, uint32_t ciphertext_size, uint64_t* out_a, uint64_t* out_b) {
  uint64_t a = 1469598103934665603ULL ^ 0x3d6f0a9b8c71e245ULL;
  a = fnv_update_u64(a, (uint64_t)section_type);
  a = fnv_update_u64(a, name_digest);
  a = fnv_update_u64(a, plain_size);
  a = fnv_update_u64(a, nonce_a);
  a = fnv_update_u64(a, nonce_b);
  a = fnv_update(a, ciphertext, ciphertext_size);
  a = seal_mix64(a ^ 0x9b62d14fa7c8305dULL);

  uint64_t b = 1469598103934665603ULL ^ 0x6c8e9cf570932bd5ULL;
  b = fnv_update_u64(b, nonce_b);
  b = fnv_update_u64(b, nonce_a);
  b = fnv_update_u64(b, plain_size);
  b = fnv_update_u64(b, name_digest);
  b = fnv_update_u64(b, (uint64_t)section_type);
  b = fnv_update(b, ciphertext, ciphertext_size);
  b = seal_mix64(b ^ 0xa41bc927f35d69e1ULL);
  *out_a = a;
  *out_b = b;
}

static uint64_t seal_next_word(uint64_t* state) {
  *state += 0x9e3779b97f4a7c15ULL;
  return seal_mix64(*state);
}

static int open_legacy_sealed_section(const unsigned char* input,
                                      uint32_t input_size,
                                      uint32_t section_type,
                                      unsigned char* out,
                                      uint32_t out_cap,
                                      uint32_t* out_size) {
  if (!input || !out || !out_size || input_size < 40u) return ERR_FORMAT;
  if (!mem_eq(input, "VSEAL001", 8u)) return ERR_FORMAT;
  const uint32_t version = read_u32(input + 8u);
  const uint32_t sealed_type = read_u32(input + 12u);
  const uint64_t plain_size64 = read_u64(input + 16u);
  const uint64_t plain_hash = read_u64(input + 24u);
  const uint64_t sealed_name_hash = read_u64(input + 32u);
  if (version != 1u) return ERR_VERSION;
  if (sealed_type != section_type) return ERR_SECTION;
  if (plain_size64 > 0xffffffffULL) return ERR_RANGE;
  const uint32_t plain_size = (uint32_t)plain_size64;
  if (plain_size != input_size - 40u) return ERR_SECTION;
  if (plain_size > out_cap) return ERR_CAPACITY;
  uint64_t state = seal_derive_seed(section_type, sealed_name_hash, plain_size64, plain_hash);
  uint64_t word = 0;
  for (uint32_t i = 0; i < plain_size; ++i) {
    if ((i & 7u) == 0u) word = seal_next_word(&state);
    out[i] = (unsigned char)(input[40u + i] ^ (unsigned char)((word >> ((i & 7u) * 8u)) & 0xffu));
  }
  if (fnv_bytes(out, plain_size) != plain_hash) return ERR_HASH;
  *out_size = plain_size;
  return ERR_OK;
}

static int open_sealed_section(const unsigned char* input,
                               uint32_t input_size,
                               uint32_t section_type,
                               unsigned char* out,
                               uint32_t out_cap,
                               uint32_t* out_size) {
  if (!input || !out || !out_size || input_size < 8u) return ERR_FORMAT;
  if (mem_eq(input, "VSEAL001", 8u)) {
    return open_legacy_sealed_section(input, input_size, section_type, out, out_cap, out_size);
  }
  if (input_size < 64u || !mem_eq(input, "VAEAD001", 8u)) return ERR_FORMAT;
  const uint32_t version = read_u32(input + 8u);
  const uint32_t sealed_type = read_u32(input + 12u);
  const uint64_t plain_size64 = read_u64(input + 16u);
  const uint64_t sealed_name_hash = read_u64(input + 24u);
  const uint64_t nonce_a = read_u64(input + 32u);
  const uint64_t nonce_b = read_u64(input + 40u);
  const uint64_t tag_a = read_u64(input + 48u);
  const uint64_t tag_b = read_u64(input + 56u);
  if (version != 1u) return ERR_VERSION;
  if (sealed_type != section_type) return ERR_SECTION;
  if (plain_size64 > 0xffffffffULL) return ERR_RANGE;
  const uint32_t plain_size = (uint32_t)plain_size64;
  if (plain_size != input_size - 64u) return ERR_SECTION;
  if (plain_size > out_cap) return ERR_CAPACITY;

  uint64_t expected_a = 0;
  uint64_t expected_b = 0;
  aead_tag(section_type, sealed_name_hash, plain_size64, nonce_a, nonce_b, input + 64u, plain_size, &expected_a, &expected_b);
  if (expected_a != tag_a || expected_b != tag_b) return ERR_HASH;

  uint64_t state = aead_stream_seed(section_type, sealed_name_hash, plain_size64, nonce_a, nonce_b);
  uint64_t word = 0;
  for (uint32_t i = 0; i < plain_size; ++i) {
    if ((i & 7u) == 0u) word = seal_next_word(&state);
    out[i] = (unsigned char)(input[64u + i] ^ (unsigned char)((word >> ((i & 7u) * 8u)) & 0xffu));
  }
  *out_size = plain_size;
  return ERR_OK;
}


static int range_ok(uint32_t offset, uint32_t size, uint32_t total) {
  return offset <= total && size <= total && offset + size >= offset && offset + size <= total;
}

static int decompress_rle(const unsigned char* input, uint32_t input_size, uint32_t expected_size, unsigned char* out, uint32_t out_cap) {
  if (!input || !out || input_size < 20u || expected_size > out_cap) return ERR_COMPRESS;
  if (!mem_eq(input, "VCLZ0008", 8u)) return ERR_COMPRESS;
  if (read_u32(input + 8u) != 1u) return ERR_COMPRESS;
  if (read_u64(input + 12u) != (uint64_t)expected_size) return ERR_COMPRESS;
  uint32_t ip = 20u;
  uint32_t op = 0u;
  while (ip < input_size) {
    const unsigned char token = input[ip++];
    if ((token & 0x80u) != 0u) {
      const uint32_t len = (uint32_t)(token & 0x7fu) + 4u;
      if (ip + 2u > input_size) return ERR_COMPRESS;
      const uint32_t dist = (uint32_t)input[ip] | ((uint32_t)input[ip + 1u] << 8u);
      ip += 2u;
      if (dist == 0u || dist > op || op + len > expected_size) return ERR_COMPRESS;
      const uint32_t src = op - dist;
      for (uint32_t j = 0; j < len; ++j) out[op++] = out[src + j];
    } else {
      const uint32_t len = (uint32_t)token + 1u;
      if (ip + len > input_size || op + len > expected_size) return ERR_COMPRESS;
      copy_bytes(out + op, input + ip, len);
      ip += len;
      op += len;
    }
  }
  return op == expected_size ? ERR_OK : ERR_COMPRESS;
}

struct SectionRef {
  uint32_t type;
  uint32_t flags;
  uint32_t name_offset;
  uint32_t name_size;
  uint32_t data_offset;
  uint32_t data_size;
  uint32_t raw_size;
  uint64_t hash;
};

struct PackageHeader {
  uint32_t size;
  uint32_t flags;
  uint32_t section_count;
  uint32_t section_table_offset;
  uint32_t section_table_size;
  uint32_t name_table_offset;
  uint32_t name_table_size;
  uint32_t payload_offset;
  uint32_t payload_size;
};

static struct PackageHeader g_package_header;

static int parse_header(const unsigned char* bytes, uint32_t size, struct PackageHeader* h) {
  if (!bytes || !h) return ERR_ARGUMENT;
  if (size < VENOM_HEADER_SIZE) return ERR_TRUNCATED;
  if (!mem_eq(bytes, "VENOMVBC", 8u)) return ERR_MAGIC;
  if (read_u32(bytes + 8u) != VENOM_PACKAGE_VERSION) return ERR_VERSION;
  if (read_u32(bytes + 12u) != VENOM_RUNTIME_ABI) return ERR_ABI;
  h->flags = read_u32(bytes + 16u);
  h->section_count = read_u32(bytes + 20u);
  const uint64_t section_table_offset64 = read_u64(bytes + 24u);
  const uint64_t section_table_size64 = read_u64(bytes + 32u);
  const uint64_t name_table_offset64 = read_u64(bytes + 40u);
  const uint64_t name_table_size64 = read_u64(bytes + 48u);
  const uint64_t payload_offset64 = read_u64(bytes + 56u);
  const uint64_t payload_size64 = read_u64(bytes + 64u);
  if (section_table_offset64 > 0xffffffffULL || section_table_size64 > 0xffffffffULL ||
      name_table_offset64 > 0xffffffffULL || name_table_size64 > 0xffffffffULL ||
      payload_offset64 > 0xffffffffULL || payload_size64 > 0xffffffffULL) return ERR_RANGE;
  h->section_table_offset = (uint32_t)section_table_offset64;
  h->section_table_size = (uint32_t)section_table_size64;
  h->name_table_offset = (uint32_t)name_table_offset64;
  h->name_table_size = (uint32_t)name_table_size64;
  h->payload_offset = (uint32_t)payload_offset64;
  h->payload_size = (uint32_t)payload_size64;
  const uint64_t package_hash = read_u64(bytes + VENOM_PACKAGE_HASH_OFFSET);
  h->size = size;
  if (h->section_count == 0u || h->section_count > VENOM_MAX_SECTIONS) return ERR_RANGE;
  if (h->section_count > 0xffffffffu / VENOM_SECTION_ENTRY_SIZE) return ERR_RANGE;
  if (h->section_table_offset != VENOM_HEADER_SIZE) return ERR_RANGE;
  if (h->section_table_size != h->section_count * VENOM_SECTION_ENTRY_SIZE) return ERR_RANGE;
  if (!range_ok(h->section_table_offset, h->section_table_size, size)) return ERR_RANGE;
  if (!range_ok(h->name_table_offset, h->name_table_size, size)) return ERR_RANGE;
  if (!range_ok(h->payload_offset, h->payload_size, size)) return ERR_RANGE;
  if (fnv_package(bytes, size) != package_hash) return ERR_HASH;
  if ((h->flags & VENOM_PACKAGE_FLAG_RELEASE) != 0u) {
    if ((h->flags & VENOM_PACKAGE_FLAG_POLYMORPHIC) == 0u) return ERR_FLAGS;
    if ((h->flags & VENOM_PACKAGE_FLAG_INTEGRITY_METADATA) == 0u) return ERR_FLAGS;
    if ((h->flags & VENOM_PACKAGE_FLAG_RUNTIME_HARDENED) == 0u) return ERR_FLAGS;
  }
  return ERR_OK;
}

static int section_at(const unsigned char* bytes, const struct PackageHeader* h, uint32_t index, struct SectionRef* out) {
  if (!h || !out || index >= h->section_count) return ERR_ARGUMENT;
  const uint32_t base = h->section_table_offset + index * VENOM_SECTION_ENTRY_SIZE;
  out->type = read_u32(bytes + base + 0u);
  out->flags = read_u32(bytes + base + 4u);
  out->name_offset = read_u32(bytes + base + 8u);
  out->name_size = read_u32(bytes + base + 12u);
  uint64_t data_offset = read_u64(bytes + base + 16u);
  uint64_t data_size = read_u64(bytes + base + 24u);
  uint64_t raw_size = read_u64(bytes + base + 32u);
  out->hash = read_u64(bytes + base + 40u);
  if (out->type == 0u || out->type > VENOM_SECTION_TYPE_MAX) return ERR_SECTION;
  if (data_offset > 0xffffffffULL || data_size > 0xffffffffULL || raw_size > 0xffffffffULL) return ERR_RANGE;
  out->data_offset = (uint32_t)data_offset;
  out->data_size = (uint32_t)data_size;
  out->raw_size = (uint32_t)raw_size;
  if ((out->flags & ~(VENOM_SECTION_COMPRESSED | VENOM_SECTION_ENCRYPTED)) != 0u) return ERR_FLAGS;
  if (out->name_offset > h->name_table_size || out->name_size > h->name_table_size - out->name_offset) return ERR_RANGE;
  if (!range_ok(h->name_table_offset + out->name_offset, out->name_size, h->size)) return ERR_RANGE;
  if (!range_ok(out->data_offset, out->data_size, h->size)) return ERR_RANGE;
  if (out->data_offset < h->payload_offset || out->data_offset + out->data_size > h->payload_offset + h->payload_size) return ERR_RANGE;
  if ((out->flags & VENOM_SECTION_ENCRYPTED) == 0u) {
    if ((out->flags & VENOM_SECTION_COMPRESSED) != 0u) {
      if (out->raw_size < out->data_size) return ERR_COMPRESS;
    } else if (out->raw_size != out->data_size) {
      return ERR_SECTION;
    }
  }
  if (fnv_bytes(bytes + out->data_offset, out->data_size) != out->hash) return ERR_HASH;
  return ERR_OK;
}

static int find_section(const unsigned char* bytes, const struct PackageHeader* h, uint32_t type, const char* name, struct SectionRef* out) {
  for (uint32_t i = 0; i < h->section_count; ++i) {
    struct SectionRef s;
    int rc = section_at(bytes, h, i, &s);
    if (rc != ERR_OK) return rc;
    const unsigned char* nm = bytes + h->name_table_offset + s.name_offset;
    if (s.type == type && section_name_eq_bytes(type, nm, s.name_size, name)) {
      *out = s;
      return ERR_OK;
    }
  }
  return ERR_MISSING;
}


static void write_u32(unsigned char* p, uint32_t v) {
  p[0] = (unsigned char)(v & 0xffu); p[1] = (unsigned char)((v >> 8u) & 0xffu);
  p[2] = (unsigned char)((v >> 16u) & 0xffu); p[3] = (unsigned char)((v >> 24u) & 0xffu);
}

static void write_u64(unsigned char* p, uint64_t v) {
  for (uint32_t i = 0; i < 8u; ++i) p[i] = (unsigned char)((v >> (i * 8u)) & 0xffu);
}

static int validate_all_sections(const unsigned char* bytes, const struct PackageHeader* h) {
  for (uint32_t i = 0; i < h->section_count; ++i) {
    struct SectionRef a;
    int rc = section_at(bytes, h, i, &a);
    if (rc != ERR_OK) return rc;
    if (a.name_size == 0u) return ERR_SECTION;
    for (uint32_t j = 0; j < i; ++j) {
      struct SectionRef b;
      rc = section_at(bytes, h, j, &b);
      if (rc != ERR_OK) return rc;
      const uint32_t a_end = a.data_offset + a.data_size;
      const uint32_t b_end = b.data_offset + b.data_size;
      if (a.data_size != 0u && b.data_size != 0u && a.data_offset < b_end && b.data_offset < a_end) return ERR_RANGE;
      if (a.type == b.type && a.name_size == b.name_size) {
        const unsigned char* an = bytes + h->name_table_offset + a.name_offset;
        const unsigned char* bn = bytes + h->name_table_offset + b.name_offset;
        int same = 1;
        for (uint32_t k = 0; k < a.name_size; ++k) if (an[k] != bn[k]) { same = 0; break; }
        if (same) return ERR_SECTION;
      }
    }
  }
  return ERR_OK;
}

static int serialize_package_metadata(const unsigned char* bytes, const struct PackageHeader* h) {
  const uint32_t header_size = 80u;
  const uint32_t table_size = h->section_count * VENOM_SECTION_ENTRY_SIZE;
  if (header_size + table_size < header_size) return ERR_CAPACITY;
  if (h->name_table_size > PACKAGE_META_CAP - header_size - table_size) return ERR_CAPACITY;
  const uint32_t total = header_size + table_size + h->name_table_size;
  memset(g_package_meta, 0, total);
  copy_bytes(g_package_meta, (const unsigned char*)"VPKG0002", 8u);
  write_u32(g_package_meta + 8u, 2u);
  write_u32(g_package_meta + 12u, VENOM_PACKAGE_VERSION);
  write_u32(g_package_meta + 16u, VENOM_RUNTIME_ABI);
  write_u32(g_package_meta + 20u, h->flags);
  write_u32(g_package_meta + 24u, h->section_count);
  write_u32(g_package_meta + 28u, header_size);
  write_u32(g_package_meta + 32u, table_size);
  write_u32(g_package_meta + 36u, header_size + table_size);
  write_u32(g_package_meta + 40u, h->name_table_size);
  write_u32(g_package_meta + 44u, h->payload_offset);
  write_u32(g_package_meta + 48u, h->payload_size);
  write_u32(g_package_meta + 52u, h->size);
  write_u64(g_package_meta + 56u, read_u64(bytes + VENOM_PACKAGE_HASH_OFFSET));
  for (uint32_t i = 0; i < h->section_count; ++i) {
    struct SectionRef sec;
    int rc = section_at(bytes, h, i, &sec);
    if (rc != ERR_OK) return rc;
    unsigned char* e = g_package_meta + header_size + i * VENOM_SECTION_ENTRY_SIZE;
    write_u32(e + 0u, sec.type); write_u32(e + 4u, sec.flags);
    write_u32(e + 8u, sec.name_offset); write_u32(e + 12u, sec.name_size);
    write_u64(e + 16u, sec.data_offset); write_u64(e + 24u, sec.data_size);
    write_u64(e + 32u, sec.raw_size); write_u64(e + 40u, sec.hash);
  }
  copy_bytes(g_package_meta + header_size + table_size, bytes + h->name_table_offset, h->name_table_size);
  g_package_meta_size = total;
  return ERR_OK;
}

static int decode_section(const unsigned char* bytes, const struct SectionRef* s, unsigned char* out, uint32_t out_cap, uint32_t* out_size) {
  if (!bytes || !s || !out || !out_size) return ERR_ARGUMENT;
  const unsigned char* staged = bytes + s->data_offset;
  uint32_t staged_size = s->data_size;
  if ((s->flags & VENOM_SECTION_ENCRYPTED) != 0u) {
    int rc = open_sealed_section(staged, staged_size, s->type, g_decode_stage, PACKAGE_CAP, &staged_size);
    if (rc != ERR_OK) return rc;
    staged = g_decode_stage;
  }
  if ((s->flags & VENOM_SECTION_COMPRESSED) != 0u) {
    int rc = decompress_rle(staged, staged_size, s->raw_size, out, out_cap);
    if (rc != ERR_OK) return rc;
    *out_size = s->raw_size;
    return ERR_OK;
  }
  if (staged_size > out_cap || s->raw_size != staged_size) return ERR_CAPACITY;
  copy_bytes(out, staged, staged_size);
  *out_size = staged_size;
  return ERR_OK;
}

static uint32_t string_stream_next(uint32_t* state) {
  *state ^= *state << 13u;
  *state ^= *state >> 17u;
  *state ^= *state << 5u;
  return *state;
}

static int decode_string_table_in_place(unsigned char* data, uint32_t size) {
  if (!data || size < 20u || !mem_eq(data, "VSTR0011", 8u)) return ERR_FORMAT;
  if (read_u32(data + 8u) != 2u) return ERR_VERSION;
  const uint32_t count = read_u32(data + 12u);
  const uint32_t table_seed = read_u32(data + 16u);
  const uint32_t entry_base = 20u;
  const uint32_t data_base = entry_base + count * 12u;
  if (data_base < entry_base || data_base > size) return ERR_RANGE;
  for (uint32_t id = 0u; id < count; ++id) {
    const uint32_t entry = entry_base + id * 12u;
    const uint32_t off = read_u32(data + entry);
    const uint32_t slen = read_u32(data + entry + 4u);
    const uint32_t nonce = read_u32(data + entry + 8u);
    if (data_base + off + slen < data_base + off || data_base + off + slen > size) return ERR_RANGE;
    uint32_t state = table_seed ^ nonce ^ (slen * 0x27D4EB2Du);
    for (uint32_t i = 0u; i < slen; ++i) data[data_base + off + i] ^= (unsigned char)(string_stream_next(&state) & 0xffu);
  }
  return ERR_OK;
}

static int get_string(uint32_t id, const unsigned char* data, uint32_t size, const unsigned char** ptr, uint32_t* len) {
  if (!data || size < 20u || !mem_eq(data, "VSTR0011", 8u)) return ERR_FORMAT;
  if (read_u32(data + 8u) != 2u) return ERR_VERSION;
  const uint32_t count = read_u32(data + 12u);
  if (id >= count) return ERR_RANGE;
  const uint32_t entry_base = 20u;
  const uint32_t data_base = entry_base + count * 12u;
  if (data_base > size) return ERR_RANGE;
  const uint32_t entry = entry_base + id * 12u;
  const uint32_t off = read_u32(data + entry);
  const uint32_t slen = read_u32(data + entry + 4u);
  if (data_base + off + slen < data_base + off || data_base + off + slen > size) return ERR_RANGE;
  *ptr = data + data_base + off;
  *len = slen;
  return ERR_OK;
}

static uint32_t string_count(const unsigned char* data, uint32_t size) {
  if (!data || size < 20u || !mem_eq(data, "VSTR0011", 8u)) return 0u;
  return read_u32(data + 12u);
}

static uint32_t route_count(const unsigned char* data, uint32_t size) {
  if (!data || size < 16u || !mem_eq(data, "VRTE0003", 8u)) return 0u;
  return read_u32(data + 12u);
}

static int route_entry_full(const unsigned char* data, uint32_t size, uint32_t index, uint32_t* route_id, uint32_t* source_id, uint32_t* bc_off, uint32_t* bc_size, uint32_t* instr_count, uint32_t* flags) {
  if (!data || size < 16u || !mem_eq(data, "VRTE0003", 8u)) return ERR_FORMAT;
  if (read_u32(data + 8u) != 1u) return ERR_VERSION;
  const uint32_t count = read_u32(data + 12u);
  if (index >= count) return ERR_RANGE;
  const uint32_t base = 16u + index * 24u;
  if (base + 24u > size) return ERR_RANGE;
  *route_id = read_u32(data + base + 0u);
  *source_id = read_u32(data + base + 4u);
  *bc_off = read_u32(data + base + 8u);
  *bc_size = read_u32(data + base + 12u);
  *instr_count = read_u32(data + base + 16u);
  *flags = read_u32(data + base + 20u);
  return ERR_OK;
}

static int route_entry(const unsigned char* data, uint32_t size, uint32_t index, uint32_t* route_id, uint32_t* bc_off, uint32_t* bc_size, uint32_t* instr_count) {
  uint32_t source_id=0u, flags=0u;
  return route_entry_full(data,size,index,route_id,&source_id,bc_off,bc_size,instr_count,&flags);
}

static int string_matches_bytes(const unsigned char* strings, uint32_t strings_size, uint32_t id, const unsigned char* value, uint32_t value_len) {
  const unsigned char* ptr = 0;
  uint32_t len = 0;
  if (get_string(id, strings, strings_size, &ptr, &len) != ERR_OK) return 0;
  if (len != value_len) return 0;
  for (uint32_t i = 0; i < len; ++i) if (ptr[i] != value[i]) return 0;
  return 1;
}

static int string_matches_ascii(const unsigned char* strings, uint32_t strings_size, uint32_t id, const char* value) {
  return string_matches_bytes(strings, strings_size, id, (const unsigned char*)value, str_len(value));
}

static uint32_t parse_u32_token(const char* p, uint32_t n) {
  uint32_t value = 0;
  uint32_t i = 0;
  while (i < n && (p[i] == ' ' || p[i] == '\t')) ++i;
  if (i + 1u < n && p[i] == '0' && (p[i + 1u] == 'x' || p[i + 1u] == 'X')) {
    i += 2u;
    while (i < n) {
      unsigned char c = (unsigned char)p[i];
      uint32_t v;
      if (c >= '0' && c <= '9') v = c - '0';
      else if (c >= 'a' && c <= 'f') v = c - 'a' + 10u;
      else if (c >= 'A' && c <= 'F') v = c - 'A' + 10u;
      else break;
      value = value * 16u + v;
      ++i;
    }
  } else {
    while (i < n && p[i] >= '0' && p[i] <= '9') {
      value = value * 10u + (uint32_t)(p[i] - '0');
      ++i;
    }
  }
  return value;
}

static uint32_t word_from_text(const char* p, uint32_t n) {
  if (n == 6u && p[0]=='o'&&p[1]=='p'&&p[2]=='c'&&p[3]=='o'&&p[4]=='d'&&p[5]=='e') return WORD_OPCODE;
  if (n == 1u && p[0] == 'a') return WORD_A;
  if (n == 1u && p[0] == 'b') return WORD_B;
  if (n == 1u && p[0] == 'c') return WORD_C;
  return 999u;
}

static int parse_poly(const unsigned char* data, uint32_t size, uint32_t* word_layout, uint32_t* opcode_mask, uint32_t* operand_masks) {
  if (!data || size < 72u || !mem_eq(data, "VPOL0010", 8u)) return ERR_FORMAT;
  if (read_u32(data + 8u) != 2u) return ERR_VERSION;
  for (uint32_t i = 0; i < 65536u; ++i) g_phys_to_logical[i] = 0u;
  for (uint32_t i = 0; i < 65536u / 32u; ++i) g_has_phys[i] = 0u;
  *opcode_mask = read_u32(data + 20u);
  operand_masks[0] = read_u32(data + 24u);
  operand_masks[1] = read_u32(data + 28u);
  operand_masks[2] = read_u32(data + 32u);
  for (uint32_t i = 0; i < 4u; ++i) {
    const uint32_t word = data[36u + i];
    if (word > WORD_C) return ERR_FORMAT;
    word_layout[i] = word;
  }
  const uint32_t count = read_u32(data + 48u);
  for (uint32_t i = 0u; i < 7u; ++i) {
    const unsigned char* item = data + 52u + i * 2u;
    const uint32_t physical = (uint32_t)item[0] | ((uint32_t)item[1] << 8u);
    if (!physical) return ERR_FORMAT;
    for (uint32_t j = 0u; j < i; ++j) if (g_dom_logical_to_physical[j + 1u] == physical) return ERR_FORMAT;
    g_dom_logical_to_physical[i + 1u] = physical;
  }
  uint32_t seen_fields = 0u;
  for (uint32_t i = 0u; i < 3u; ++i) {
    const uint32_t field = data[66u + i];
    if (field > 2u || (seen_fields & (1u << field))) return ERR_FORMAT;
    seen_fields |= 1u << field;
    g_dom_field_layout[i] = field;
  }
  if (72u + count * 4u < 72u || 72u + count * 4u > size) return ERR_RANGE;
  for (uint32_t i = 0; i < count; ++i) {
    const unsigned char* entry = data + 72u + i * 4u;
    const uint32_t logical = (uint32_t)entry[0] | ((uint32_t)entry[1] << 8u);
    const uint32_t physical = (uint32_t)entry[2] | ((uint32_t)entry[3] << 8u);
    g_phys_to_logical[physical] = logical;
    g_has_phys[physical / 32u] |= 1u << (physical % 32u);
  }
  return count ? ERR_OK : ERR_FORMAT;
}

static int decode_instruction(const unsigned char* vm_data, uint32_t vm_size, uint32_t pc, const uint32_t* word_layout, uint32_t opcode_mask, const uint32_t* operand_masks, uint32_t* logical, uint32_t* a, uint32_t* b, uint32_t* c) {
  if (pc + 16u > vm_size) return ERR_VM;
  uint32_t words[4];
  words[0] = read_u32(vm_data + pc + 0u);
  words[1] = read_u32(vm_data + pc + 4u);
  words[2] = read_u32(vm_data + pc + 8u);
  words[3] = read_u32(vm_data + pc + 12u);
  uint32_t physical = 0;
  uint32_t aa = 0, bb = 0, cc = 0;
  for (uint32_t i = 0; i < 4u; ++i) {
    const uint32_t value = words[i];
    if (word_layout[i] == WORD_OPCODE) physical = value ^ opcode_mask;
    else if (word_layout[i] == WORD_A) aa = value ^ operand_masks[0];
    else if (word_layout[i] == WORD_B) bb = value ^ operand_masks[1];
    else if (word_layout[i] == WORD_C) cc = value ^ operand_masks[2];
    else return ERR_VM;
  }
  if (physical >= 65536u || (g_has_phys[physical / 32u] & (1u << (physical % 32u))) == 0u) return ERR_VM;
  *logical = g_phys_to_logical[physical];
  *a = aa;
  *b = bb;
  *c = cc;
  return ERR_OK;
}

static void result_reset(void) { g_result_size = 0u; }
static void result_ch(char c) { if (g_result_size + 1u < RESULT_CAP) g_result[g_result_size++] = (unsigned char)c; }
static void result_str(const char* s) { for (uint32_t i = 0; s && s[i]; ++i) result_ch(s[i]); }
static void result_raw(const unsigned char* p, uint32_t n) { for (uint32_t i = 0; i < n; ++i) result_ch((char)p[i]); }

static void dom_reset(void) { g_domops_size = 0u; g_domops_overflow = 0; }
static void dom_ch(char c) { if (g_domops_size + 1u < DOMOPS_CAP) g_domops[g_domops_size++] = (unsigned char)c; else g_domops_overflow = 1; }
static void dom_str(const char* s) { for (uint32_t i = 0; s && s[i]; ++i) dom_ch(s[i]); }
static void dom_json_bytes(const unsigned char* p, uint32_t n) {
  for (uint32_t i = 0; i < n; ++i) {
    unsigned char c = p[i];
    if (c == '"' || c == '\\') { dom_ch('\\'); dom_ch((char)c); }
    else if (c == '\n') { dom_ch('\\'); dom_ch('n'); }
    else if (c == '\r') { dom_ch('\\'); dom_ch('r'); }
    else if (c == '\t') { dom_ch('\\'); dom_ch('t'); }
    else if (c >= 32u && c < 127u) dom_ch((char)c);
  }
}
static void dom_op_start(uint32_t* first, const char* op) {
  if (!*first) dom_ch(',');
  *first = 0u;
  dom_str("{\"op\":\"");
  dom_str(op);
  dom_ch('\"');
}
static void dom_field_bytes(const char* key, const unsigned char* value, uint32_t value_len) {
  dom_str(",\"");
  dom_str(key);
  dom_str("\":\"");
  dom_json_bytes(value, value_len);
  dom_ch('\"');
}
static void dom_op_end(void) { dom_ch('}'); }
static int is_event_attr(const unsigned char* name, uint32_t len) {
  if (!name || len < 3u) return 0;
  if (name[0] != 'o' || name[1] != 'n') return 0;
  unsigned char c = name[2];
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void dom_bin_ch(unsigned char c) { if (g_domop_bin_size < DOMOP_BIN_CAP) g_domop_bin[g_domop_bin_size++] = c; else g_domop_bin_overflow = 1; }
static void dom_bin_u32(uint32_t v) {
  dom_bin_ch((unsigned char)(v & 0xffu));
  dom_bin_ch((unsigned char)((v >> 8u) & 0xffu));
  dom_bin_ch((unsigned char)((v >> 16u) & 0xffu));
  dom_bin_ch((unsigned char)((v >> 24u) & 0xffu));
}
static void dom_bin_bytes(const unsigned char* p, uint32_t n) { for (uint32_t i = 0; i < n; ++i) dom_bin_ch(p[i]); }
static void dom_bin_reset(void) {
  g_domop_bin_size = 0u;
  g_domop_bin_overflow = 0;
  dom_bin_bytes((const unsigned char*)"VDOP0020", 8u);
  dom_bin_u32(2u);
  dom_bin_u32(0u);
  dom_bin_u32(7u);
  for (uint32_t logical = 1u; logical <= 7u; ++logical) {
    dom_bin_u32(logical);
    dom_bin_u32(g_dom_logical_to_physical[logical]);
  }
  dom_bin_ch((unsigned char)g_dom_field_layout[0]);
  dom_bin_ch((unsigned char)g_dom_field_layout[1]);
  dom_bin_ch((unsigned char)g_dom_field_layout[2]);
  dom_bin_ch(0u);
}
static void dom_bin_patch_count(uint32_t count) {
  if (g_domop_bin_size >= 16u) {
    g_domop_bin[12] = (unsigned char)(count & 0xffu);
    g_domop_bin[13] = (unsigned char)((count >> 8u) & 0xffu);
    g_domop_bin[14] = (unsigned char)((count >> 16u) & 0xffu);
    g_domop_bin[15] = (unsigned char)((count >> 24u) & 0xffu);
  }
}
static void dom_bin_op(uint32_t logical_op, const unsigned char* a, uint32_t a_len, const unsigned char* b, uint32_t b_len, const unsigned char* c, uint32_t c_len) {
  const unsigned char* values[3] = {a, b, c};
  const uint32_t lengths[3] = {a_len, b_len, c_len};
  if (logical_op == 0u || logical_op > 7u) { g_domop_bin_overflow = 1; return; }
  dom_bin_u32(g_dom_logical_to_physical[logical_op]);
  for (uint32_t i = 0u; i < 3u; ++i) dom_bin_u32(lengths[g_dom_field_layout[i]]);
  for (uint32_t i = 0u; i < 3u; ++i) {
    const uint32_t field = g_dom_field_layout[i];
    if (lengths[field]) dom_bin_bytes(values[field], lengths[field]);
  }
}

static void result_u32(uint32_t v) {
  char tmp[16]; uint32_t n = 0;
  if (!v) { result_ch('0'); return; }
  while (v && n < sizeof(tmp)) { tmp[n++] = (char)('0' + (v % 10u)); v /= 10u; }
  while (n) result_ch(tmp[--n]);
}
static void result_json_bytes(const unsigned char* p, uint32_t n) {
  for (uint32_t i = 0; i < n; ++i) {
    unsigned char c = p[i];
    if (c == '"' || c == '\\') { result_ch('\\'); result_ch((char)c); }
    else if (c == '\n') { result_ch('\\'); result_ch('n'); }
    else if (c == '\r') { result_ch('\\'); result_ch('r'); }
    else if (c == '\t') { result_ch('\\'); result_ch('t'); }
    else if (c >= 32u && c < 127u) result_ch((char)c);
  }
}
static void result_text_append(const unsigned char* p, uint32_t n, unsigned char* text, uint32_t* text_len, uint32_t cap) {
  for (uint32_t i = 0; i < n && *text_len + 1u < cap; ++i) text[(*text_len)++] = p[i];
}

static uint32_t count_script_chunks(const unsigned char* data, uint32_t size) {
  if (!data || size < 16u || !mem_eq(data, "VJSB0006", 8u)) return 0u;
  return read_u32(data + 12u);
}

static uint32_t count_assets(const unsigned char* data, uint32_t size) {
  if (!data || !size) return 0u;
  uint32_t lines = 0u;
  for (uint32_t i = 0; i < size; ++i) if (data[i] == '\n') ++lines;
  return lines > 4u ? lines - 4u : 0u;
}

__attribute__((export_name("venom_runtime_abi"))) unsigned int venom_runtime_abi(void) { return VENOM_RUNTIME_ABI; }
__attribute__((export_name("venom_package_version"))) unsigned int venom_package_version(void) { return VENOM_PACKAGE_VERSION; }
static unsigned int venom_wasm_package_ptr(void) { return (unsigned int)(uintptr_t)g_package; }
static unsigned int venom_wasm_package_capacity(void) { return PACKAGE_CAP; }
static unsigned int venom_wasm_route_ptr(void) { return (unsigned int)(uintptr_t)g_route; }
static unsigned int venom_wasm_route_capacity(void) { return ROUTE_CAP; }
static unsigned int venom_wasm_result_ptr(void) { return (unsigned int)(uintptr_t)g_result; }
static unsigned int venom_wasm_result_capacity(void) { return RESULT_CAP; }
static unsigned int venom_wasm_result_size(void) { return g_result_size; }
static unsigned int venom_wasm_domops_ptr(void) { return (unsigned int)(uintptr_t)g_domop_bin; }
static unsigned int venom_wasm_domops_capacity(void) { return DOMOP_BIN_CAP; }
static unsigned int venom_wasm_domops_size(void) { return g_domop_bin_size; }
static unsigned int venom_wasm_materialized_section_ptr(void) { return (unsigned int)(uintptr_t)g_materialized_section; }
static unsigned int venom_wasm_materialized_section_capacity(void) { return PACKAGE_CAP; }
static unsigned int venom_wasm_materialized_section_size(void) { return g_materialized_section_size; }
static unsigned int venom_wasm_package_meta_ptr(void) { return (unsigned int)(uintptr_t)g_package_meta; }
static unsigned int venom_wasm_package_meta_capacity(void) { return PACKAGE_META_CAP; }
static unsigned int venom_wasm_package_meta_size(void) { return g_package_meta_size; }
static unsigned int venom_wasm_route_record_ptr(void) { return (unsigned int)(uintptr_t)g_route_record; }
static unsigned int venom_wasm_route_record_capacity(void) { return ROUTE_RECORD_CAP; }
static unsigned int venom_wasm_route_record_size(void) { return g_route_record_size; }

static int venom_wasm_parse_package(unsigned int package_size) {
  g_package_meta_size = 0u; g_package_loaded = 0; g_package_loaded_size = 0u;
  if (package_size > PACKAGE_CAP) return ERR_CAPACITY;
  struct PackageHeader h;
  int rc = parse_header(g_package, package_size, &h);
  if (rc != ERR_OK) return rc;
  rc = validate_all_sections(g_package, &h);
  if (rc != ERR_OK) return rc;
  rc = serialize_package_metadata(g_package, &h);
  if (rc != ERR_OK) return rc;
  g_package_header = h; g_package_loaded_size = package_size; g_package_loaded = 1;
  return ERR_OK;
}

static int venom_wasm_resolve_route(unsigned int package_size, unsigned int route_size) {
  g_route_record_size=0u;
  if (package_size>PACKAGE_CAP || route_size>ROUTE_CAP) return ERR_CAPACITY;
  struct PackageHeader h; int rc;
  if (g_package_loaded && g_package_loaded_size==package_size) h=g_package_header;
  else { rc=parse_header(g_package,package_size,&h); if(rc!=ERR_OK)return rc; rc=validate_all_sections(g_package,&h); if(rc!=ERR_OK)return rc; }
  struct SectionRef sec; uint32_t strings_size=0u,routes_size=0u;
  rc=find_section(g_package,&h,VENOM_SECTION_STRINGS,"strings.vstr",&sec); if(rc!=ERR_OK)return rc;
  rc=decode_section(g_package,&sec,g_strings,STRINGS_CAP,&strings_size); if(rc!=ERR_OK)return rc; rc=decode_string_table_in_place(g_strings,strings_size); if(rc!=ERR_OK)return rc;
  rc=find_section(g_package,&h,VENOM_SECTION_ROUTES,"routes.vbrt",&sec); if(rc!=ERR_OK)return rc;
  rc=decode_section(g_package,&sec,g_routes,ROUTES_CAP,&routes_size); if(rc!=ERR_OK)return rc;
  unsigned char normalized[ROUTE_CAP]; uint32_t normalized_size=normalize_route_bytes(g_route,route_size,normalized,ROUTE_CAP);
  uint32_t count=route_count(g_routes,routes_size), selected=0xffffffffu, root=0xffffffffu;
  for(uint32_t i=0u;i<count;++i){ uint32_t rid,sid,off,sz,ic,fl; rc=route_entry_full(g_routes,routes_size,i,&rid,&sid,&off,&sz,&ic,&fl); if(rc!=ERR_OK)return rc; if(string_matches_bytes(g_strings,strings_size,rid,normalized,normalized_size)){selected=i;break;} if(string_matches_ascii(g_strings,strings_size,rid,"/"))root=i; }
  if(selected==0xffffffffu) selected=(root!=0xffffffffu)?root:0u; if(selected>=count)return ERR_MISSING;
  uint32_t rid,sid,off,sz,ic,fl; rc=route_entry_full(g_routes,routes_size,selected,&rid,&sid,&off,&sz,&ic,&fl); if(rc!=ERR_OK)return rc;
  const unsigned char *rp=0,*sp=0; uint32_t rn=0u,sn=0u; if(get_string(rid,g_strings,strings_size,&rp,&rn)!=ERR_OK)return ERR_FORMAT; if(get_string(sid,g_strings,strings_size,&sp,&sn)!=ERR_OK)return ERR_FORMAT;
  if(48u+rn+sn>ROUTE_RECORD_CAP)return ERR_CAPACITY; memcpy(g_route_record,"VROUTE01",8u); write_u32(g_route_record+8u,1u); write_u32(g_route_record+12u,selected); write_u32(g_route_record+16u,rid); write_u32(g_route_record+20u,sid); write_u32(g_route_record+24u,off); write_u32(g_route_record+28u,sz); write_u32(g_route_record+32u,ic); write_u32(g_route_record+36u,fl); write_u32(g_route_record+40u,rn); write_u32(g_route_record+44u,sn); memcpy(g_route_record+48u,rp,rn); memcpy(g_route_record+48u+rn,sp,sn); g_route_record_size=48u+rn+sn; return ERR_OK;
}

static int venom_wasm_materialize_section(unsigned int package_size,
                                                                                       unsigned int section_index,
                                                                                       unsigned int expected_type,
                                                                                       unsigned int expected_raw_size) {
  venom_wasm_release_materialized();
  if (package_size > PACKAGE_CAP || expected_type == 0u || expected_type > VENOM_SECTION_TYPE_MAX) return ERR_CAPACITY;
  struct PackageHeader h;
  int rc;
  if (g_package_loaded && g_package_loaded_size == package_size) h = g_package_header;
  else { rc = parse_header(g_package, package_size, &h); if (rc != ERR_OK) return rc; rc = validate_all_sections(g_package, &h); if (rc != ERR_OK) return rc; }
  struct SectionRef sec;
  rc = section_at(g_package, &h, section_index, &sec);
  if (rc != ERR_OK) return rc;
  /* Bind every materialization request to metadata already authenticated by the
     package parser. This prevents an index-only plaintext oracle and catches
     type/size confusion at the JS/WASM boundary. */
  if (sec.type != expected_type || sec.raw_size != expected_raw_size) return ERR_FORMAT;
  rc = decode_section(g_package, &sec, g_materialized_section, PACKAGE_CAP, &g_materialized_section_size);
  if (rc != ERR_OK) { venom_wasm_release_materialized(); return rc; }
  if (g_materialized_section_size != expected_raw_size) { venom_wasm_release_materialized(); return ERR_FORMAT; }
  return ERR_OK;
}

static int venom_wasm_analyze(unsigned int package_size, unsigned int route_size) {
  if (package_size > PACKAGE_CAP || route_size > ROUTE_CAP) return ERR_CAPACITY;
  struct PackageHeader h;
  int rc = parse_header(g_package, package_size, &h);
  if (rc != ERR_OK) return rc;

  struct SectionRef sec;
  uint32_t strings_size = 0, routes_size = 0, bytecode_size = 0, poly_size = 0, js_size = 0, asset_size = 0;
  rc = find_section(g_package, &h, VENOM_SECTION_STRINGS, "strings.vstr", &sec); if (rc != ERR_OK) return rc;
  rc = decode_section(g_package, &sec, g_strings, STRINGS_CAP, &strings_size); if (rc != ERR_OK) return rc; rc = decode_string_table_in_place(g_strings, strings_size); if (rc != ERR_OK) return rc;
  rc = find_section(g_package, &h, VENOM_SECTION_ROUTES, "routes.vbrt", &sec); if (rc != ERR_OK) return rc;
  rc = decode_section(g_package, &sec, g_routes, ROUTES_CAP, &routes_size); if (rc != ERR_OK) return rc;
  rc = find_section(g_package, &h, VENOM_SECTION_VM_BYTECODE, "route-bytecode.vmbc", &sec); if (rc != ERR_OK) return rc;
  rc = decode_section(g_package, &sec, g_bytecode, BYTECODE_CAP, &bytecode_size); if (rc != ERR_OK) return rc;
  rc = find_section(g_package, &h, VENOM_SECTION_INTEGRITY, "vm-polymorph.vpol", &sec); if (rc != ERR_OK) return rc;
  rc = decode_section(g_package, &sec, g_poly_text, POLY_CAP, &poly_size); if (rc != ERR_OK) return rc;
  if (find_section(g_package, &h, VENOM_SECTION_JAVASCRIPT, "scripts.vjsb", &sec) == ERR_OK) (void)decode_section(g_package, &sec, g_js, JS_CAP, &js_size);
  if (find_section(g_package, &h, VENOM_SECTION_ASSET_MANIFEST, "asset-manifest.txt", &sec) == ERR_OK) (void)decode_section(g_package, &sec, g_asset, ASSET_CAP, &asset_size);

  uint32_t word_layout[4], opcode_mask, operand_masks[3];
  rc = parse_poly(g_poly_text, poly_size, word_layout, &opcode_mask, operand_masks);
  if (rc != ERR_OK) return rc;

  if (bytecode_size < 24u || !mem_eq(g_bytecode, "VBCODE03", 8u)) return ERR_FORMAT;
  if (read_u32(g_bytecode + 8u) != 1u) return ERR_VERSION;
  const uint32_t instr_size = read_u32(g_bytecode + 12u);
  const uint32_t total_instr = read_u32(g_bytecode + 16u);
  if (instr_size != 16u) return ERR_FORMAT;
  const uint32_t vm_data_offset = 24u;
  const uint32_t vm_data_size = bytecode_size - vm_data_offset;

  const uint32_t routes = route_count(g_routes, routes_size);
  uint32_t selected_index = 0xffffffffu;
  uint32_t root_index = 0xffffffffu;
  for (uint32_t i = 0; i < routes; ++i) {
    uint32_t route_id, off, sz, ic;
    rc = route_entry(g_routes, routes_size, i, &route_id, &off, &sz, &ic);
    if (rc != ERR_OK) return rc;
    if (string_matches_bytes(g_strings, strings_size, route_id, g_route, route_size)) { selected_index = i; break; }
    if (string_matches_ascii(g_strings, strings_size, route_id, "/")) root_index = i;
  }
  if (selected_index == 0xffffffffu) selected_index = root_index != 0xffffffffu ? root_index : 0u;
  if (selected_index >= routes) return ERR_MISSING;

  uint32_t selected_route_id, bc_off, bc_size, icount;
  rc = route_entry(g_routes, routes_size, selected_index, &selected_route_id, &bc_off, &bc_size, &icount);
  if (rc != ERR_OK) return rc;
  if (bc_off + bc_size < bc_off || bc_off + bc_size > vm_data_size) return ERR_RANGE;

  unsigned char text[2048];
  uint32_t text_len = 0u;
  uint32_t dom_ops = 0u;
  uint32_t event_bindings = 0u;
  uint32_t first_dom_op = 1u;
  uint32_t executed = 0u;
  uint32_t pc = vm_data_offset + bc_off;
  const uint32_t end = pc + bc_size;
  int saw_halt = 0;
  dom_reset();
  dom_bin_reset();
  dom_ch('[');
  while (pc < end) {
    uint32_t logical = 0, a = 0, b = 0, c = 0;
    rc = decode_instruction(g_bytecode, bytecode_size, pc, word_layout, opcode_mask, operand_masks, &logical, &a, &b, &c);
    (void)c;
    if (rc != ERR_OK) return rc;
    pc += instr_size;
    executed++;
    if (logical == VENOM_LOGICAL_CREATE_ELEMENT) {
      const unsigned char* p = 0; uint32_t n = 0;
      rc = get_string(a, g_strings, strings_size, &p, &n);
      if (rc != ERR_OK) return rc;
      dom_op_start(&first_dom_op, "createElement");
      dom_field_bytes("tag", p, n);
      dom_op_end();
      dom_bin_op(DOMOP_CREATE_ELEMENT, p, n, 0, 0u, 0, 0u);
      dom_ops++;
    } else if (logical == VENOM_LOGICAL_ENTER_ELEMENT) {
      dom_op_start(&first_dom_op, "enterElement");
      dom_op_end();
      dom_bin_op(DOMOP_ENTER_ELEMENT, 0, 0u, 0, 0u, 0, 0u);
      dom_ops++;
    } else if (logical == VENOM_LOGICAL_SET_ATTRIBUTE) {
      const unsigned char* name = 0; uint32_t name_len = 0;
      const unsigned char* value = 0; uint32_t value_len = 0;
      rc = get_string(a, g_strings, strings_size, &name, &name_len);
      if (rc != ERR_OK) return rc;
      rc = get_string(b, g_strings, strings_size, &value, &value_len);
      if (rc != ERR_OK) return rc;
      dom_op_start(&first_dom_op, "setAttribute");
      dom_field_bytes("name", name, name_len);
      dom_field_bytes("value", value, value_len);
      dom_op_end();
      dom_bin_op(DOMOP_SET_ATTRIBUTE, name, name_len, value, value_len, 0, 0u);
      dom_ops++;
      if (is_event_attr(name, name_len)) {
        dom_op_start(&first_dom_op, "bindEvent");
        dom_field_bytes("attr", name, name_len);
        if (name_len > 2u) dom_field_bytes("event", name + 2u, name_len - 2u);
        else dom_field_bytes("event", (const unsigned char*)"", 0u);
        dom_field_bytes("handler", value, value_len);
        dom_op_end();
        dom_bin_op(DOMOP_BIND_EVENT, name, name_len, name_len > 2u ? name + 2u : (const unsigned char*)"", name_len > 2u ? name_len - 2u : 0u, value, value_len);
        dom_ops++;
        event_bindings++;
      }
    } else if (logical == VENOM_LOGICAL_SET_TEXT) {
      const unsigned char* p = 0; uint32_t n = 0;
      rc = get_string(a, g_strings, strings_size, &p, &n);
      if (rc != ERR_OK) return rc;
      result_text_append(p, n, text, &text_len, sizeof(text));
      dom_op_start(&first_dom_op, "setText");
      dom_field_bytes("value", p, n);
      dom_op_end();
      dom_bin_op(DOMOP_SET_TEXT, p, n, 0, 0u, 0, 0u);
      dom_ops++;
    } else if (logical == VENOM_LOGICAL_LEAVE_ELEMENT) {
      dom_op_start(&first_dom_op, "leaveElement");
      dom_op_end();
      dom_bin_op(DOMOP_LEAVE_ELEMENT, 0, 0u, 0, 0u, 0, 0u);
      dom_ops++;
    } else if (logical == VENOM_LOGICAL_APPEND_CHILD) {
      dom_op_start(&first_dom_op, "appendChild");
      dom_op_end();
      dom_bin_op(DOMOP_APPEND_CHILD, 0, 0u, 0, 0u, 0, 0u);
      dom_ops++;
    } else if (logical == VENOM_LOGICAL_HALT) {
      saw_halt = 1;
      break;
    } else if (logical == VENOM_LOGICAL_NOP || logical == VENOM_LOGICAL_LOAD_CONST || logical == VENOM_LOGICAL_CALL_QUICKJS) {
      continue;
    } else {
      return ERR_VM;
    }
  }
  dom_ch(']');
  dom_bin_patch_count(dom_ops);
  if (g_domops_overflow || g_domop_bin_overflow) return ERR_CAPACITY;
  if (!saw_halt) return ERR_VM;

  const unsigned char* route_ptr = 0; uint32_t route_len = 0;
  rc = get_string(selected_route_id, g_strings, strings_size, &route_ptr, &route_len);
  if (rc != ERR_OK) return rc;

  result_reset();
  result_ch('{');
  result_str("\"version\":"); result_u32(VENOM_PACKAGE_VERSION);
  result_str(",\"abi\":"); result_u32(VENOM_RUNTIME_ABI);
  result_str(",\"sections\":"); result_u32(h.section_count);
  result_str(",\"routes\":"); result_u32(routes);
  result_str(",\"strings\":"); result_u32(string_count(g_strings, strings_size));
  result_str(",\"instructions\":"); result_u32(total_instr);
  result_str(",\"executed\":"); result_u32(executed);
  result_str(",\"scripts\":"); result_u32(count_script_chunks(g_js, js_size));
  result_str(",\"assets\":"); result_u32(count_assets(g_asset, asset_size));
  result_str(",\"hostBridgeVersion\":1");
  result_str(",\"eventBindings\":"); result_u32(event_bindings);
  result_str(",\"route\":\""); result_json_bytes(route_ptr, route_len); result_ch('"');
  result_str(",\"text\":\""); result_json_bytes(text, text_len); result_ch('"');
  result_str(",\"domOpCount\":"); result_u32(dom_ops);
  result_str(",\"domOpFormat\":\"VDOP0020\"");
  result_str(",\"domOpBytes\":"); result_u32(g_domop_bin_size);
  result_str(",\"domOps\":"); result_raw(g_domops, g_domops_size);
  result_ch('}');
  return ERR_OK;
}

/* v1.0.12: compact opaque package-runtime ABI.  The browser only needs three
 * generic entry points plus linear memory; descriptive helpers remain private. */
__attribute__((export_name("v12_p"))) unsigned int v12_p(unsigned int slot) {
  switch (slot) {
    case 0u: return venom_wasm_package_ptr();
    case 1u: return venom_wasm_route_ptr();
    case 2u: return venom_wasm_result_ptr();
    case 3u: return venom_wasm_domops_ptr();
    case 4u: return venom_wasm_materialized_section_ptr();
    case 5u: return venom_wasm_package_meta_ptr();
    case 6u: return venom_wasm_route_record_ptr();
    default: return 0u;
  }
}

__attribute__((export_name("v12_n"))) unsigned int v12_n(unsigned int slot) {
  switch (slot) {
    case 0u: return venom_wasm_package_capacity();
    case 1u: return venom_wasm_route_capacity();
    case 2u: return venom_wasm_result_size();
    case 3u: return venom_wasm_domops_capacity();
    case 4u: return venom_wasm_domops_size();
    case 5u: return venom_wasm_materialized_section_capacity();
    case 6u: return venom_wasm_materialized_section_size();
    case 11u: return g_route_generation;
    case 7u: return venom_wasm_package_meta_capacity();
    case 8u: return venom_wasm_package_meta_size();
    case 9u: return venom_wasm_route_record_capacity();
    case 10u: return venom_wasm_route_record_size();
    default: return 0u;
  }
}

__attribute__((export_name("v12_x"))) int v12_x(unsigned int op,
                                                  unsigned int a,
                                                  unsigned int b,
                                                  unsigned int c,
                                                  unsigned int d) {
  switch (op) {
    case 1u: return venom_wasm_parse_package(a);
    case 2u: return venom_wasm_resolve_route(a, b);
    case 3u: return venom_wasm_materialize_section(a, b, c, d);
    case 5u: venom_wasm_release_materialized(); return ERR_OK;
    case 6u: venom_wasm_release_transient(); return ERR_OK;
    case 4u: return venom_wasm_analyze(a, b);
    default: return ERR_FORMAT;
  }
}

