#include "decompiler/decompiler.hpp"

#include "compiler/pipeline/security.hpp"
#include "package/format.hpp"
#include "package/hash.hpp"
#include "package/reader.hpp"
#include "quickjs/bytecode.hpp"
#include "vm/bytecode.hpp"
#include "vm/opcode.hpp"
#include "vm/polymorph.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(VENOM_ENABLE_FULL_QUICKJS)
#include "quickjs.h"
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
#endif

namespace venom::decompiler {
namespace {

struct RecoveryStats {
  std::filesystem::path input;
  std::filesystem::path package;
  std::filesystem::path output;
  std::size_t sections = 0;
  std::size_t exact_sections = 0;
  std::size_t routes = 0;
  std::size_t vm_instructions = 0;
  std::size_t script_bundles = 0;
  std::size_t script_chunks = 0;
  std::size_t browser_sources = 0;
  std::size_t quickjs_records = 0;
  std::size_t quickjs_modules = 0;
  std::size_t quickjs_disassemblies = 0;
  std::size_t quickjs_strings = 0;
  std::size_t quickjs_identifiers = 0;
  std::size_t quickjs_urls = 0;
  std::size_t quickjs_paths = 0;
  std::size_t loose_javascript = 0;
  std::vector<std::string> warnings;
};

struct PolyMap {
  std::array<venom::vm::EncodedWord, 4> word_layout{
      venom::vm::EncodedWord::Opcode,
      venom::vm::EncodedWord::A,
      venom::vm::EncodedWord::B,
      venom::vm::EncodedWord::C};
  std::uint32_t opcode_mask = 0;
  std::array<std::uint32_t, 3> operand_masks{0, 0, 0};
  std::unordered_map<std::uint32_t, std::uint32_t> physical_to_logical;
};

struct RouteEntry {
  std::uint32_t route_id = 0;
  std::uint32_t source_id = 0;
  std::uint32_t bytecode_offset = 0;
  std::uint32_t bytecode_size = 0;
  std::uint32_t instruction_count = 0;
  std::uint32_t flags = 0;
};

struct VmImage {
  std::uint32_t instruction_size = 0;
  std::uint32_t instruction_count = 0;
  std::uint32_t flags = 0;
  std::vector<unsigned char> data;
};

struct DecodedInstruction {
  venom::vm::LogicalOpcode opcode = venom::vm::LogicalOpcode::Nop;
  std::uint32_t a = 0;
  std::uint32_t b = 0;
  std::uint32_t c = 0;
};

struct HtmlNode;
struct HtmlItem {
  std::string text;
  std::unique_ptr<HtmlNode> child;
};
struct HtmlNode {
  std::string tag;
  std::vector<std::pair<std::string, std::string>> attributes;
  std::vector<HtmlItem> items;
};

struct ScriptChunk {
  std::string route;
  std::string source;
  std::uint32_t order = 0;
  std::uint32_t flags = 0;
  std::uint32_t kind = 0;
  std::vector<unsigned char> code;
};

struct QuickJsRecord {
  bool module = false;
  std::uint32_t abi = 0;
  std::uint64_t bytecode_hash = 0;
  std::uint64_t source_hash = 0;
  std::uint32_t source_size = 0;
  std::vector<unsigned char> payload;
};

std::vector<unsigned char> read_bytes(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("failed to read " + path.string());
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void write_bytes(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) throw std::runtime_error("failed to write " + path.string());
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!out) throw std::runtime_error("failed while writing " + path.string());
}

void write_text(const std::filesystem::path& path, const std::string& text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) throw std::runtime_error("failed to write " + path.string());
  out << text;
  if (!out) throw std::runtime_error("failed while writing " + path.string());
}

bool has_magic(const std::vector<unsigned char>& bytes, const char* magic, std::size_t size = 8u) {
  return bytes.size() >= size && std::memcmp(bytes.data(), magic, size) == 0;
}

std::uint32_t read_u32(const std::vector<unsigned char>& bytes, std::size_t offset, const char* label) {
  if (offset > bytes.size() || bytes.size() - offset < 4u) throw std::runtime_error(std::string("truncated ") + label);
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
         (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
         (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

std::uint64_t read_u64(const std::vector<unsigned char>& bytes, std::size_t offset, const char* label) {
  if (offset > bytes.size() || bytes.size() - offset < 8u) throw std::runtime_error(std::string("truncated ") + label);
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < 8u; ++i) value |= static_cast<std::uint64_t>(bytes[offset + i]) << (i * 8u);
  return value;
}

std::string read_string(const std::vector<unsigned char>& bytes, std::size_t offset, std::size_t size, const char* label) {
  if (offset > bytes.size() || size > bytes.size() - offset) throw std::runtime_error(std::string("truncated ") + label);
  return std::string(reinterpret_cast<const char*>(bytes.data() + offset), size);
}

std::vector<unsigned char> slice(const std::vector<unsigned char>& bytes, std::size_t offset, std::size_t size, const char* label) {
  if (offset > bytes.size() || size > bytes.size() - offset) throw std::runtime_error(std::string("truncated ") + label);
  return std::vector<unsigned char>(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                                    bytes.begin() + static_cast<std::ptrdiff_t>(offset + size));
}

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string sanitize_component(std::string value) {
  if (value.empty()) return "unnamed";
  for (char& c : value) {
    const unsigned char u = static_cast<unsigned char>(c);
    if (!(std::isalnum(u) != 0 || c == '.' || c == '-' || c == '_')) c = '_';
  }
  while (value.find("..") != std::string::npos) value.replace(value.find(".."), 2u, "__");
  if (value.size() > 120u) value.resize(120u);
  return value;
}

std::string json_escape(const std::string& value) {
  std::ostringstream out;
  for (unsigned char c : value) {
    switch (c) {
      case '"': out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\b': out << "\\b"; break;
      case '\f': out << "\\f"; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (c < 0x20u) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned>(c) << std::dec;
        else out << static_cast<char>(c);
    }
  }
  return out.str();
}

std::string hex64(std::uint64_t value) {
  std::ostringstream out;
  out << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
  return out.str();
}

bool looks_textual(const std::vector<unsigned char>& bytes) {
  if (bytes.empty()) return true;
  std::size_t printable = 0;
  for (unsigned char c : bytes) {
    if (c == 0u) return false;
    if (c == '\n' || c == '\r' || c == '\t' || (c >= 0x20u && c < 0x7fu)) ++printable;
  }
  return printable * 100u >= bytes.size() * 88u;
}

std::string bytes_to_text(const std::vector<unsigned char>& bytes) {
  return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::filesystem::path choose_package(const std::filesystem::path& input) {
  if (std::filesystem::is_regular_file(input)) {
    const auto ext = lower(input.extension().string());
    if (ext == ".vbc" || ext == ".pax") return std::filesystem::absolute(input);
    return {};
  }
  if (!std::filesystem::is_directory(input)) return {};
  std::vector<std::filesystem::path> candidates;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(input)) {
    if (!entry.is_regular_file()) continue;
    const auto ext = lower(entry.path().extension().string());
    if (ext == ".vbc" || ext == ".pax") candidates.push_back(entry.path());
  }
  if (candidates.empty()) return {};
  std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
    const auto as = a.generic_string();
    const auto bs = b.generic_string();
    const bool aa = as.find("/assets/app/") != std::string::npos;
    const bool ba = bs.find("/assets/app/") != std::string::npos;
    if (aa != ba) return aa > ba;
    std::error_code ec1, ec2;
    const auto az = std::filesystem::file_size(a, ec1);
    const auto bz = std::filesystem::file_size(b, ec2);
    if (!ec1 && !ec2 && az != bz) return az > bz;
    return as < bs;
  });
  return std::filesystem::absolute(candidates.front());
}

void prepare_output(const std::filesystem::path& output, bool force) {
  if (std::filesystem::exists(output)) {
    if (!force) throw std::runtime_error("output already exists; use --force to replace it: " + output.string());
    std::error_code ec;
    std::filesystem::remove_all(output, ec);
    if (ec) throw std::runtime_error("failed to remove existing output: " + ec.message());
  }
  std::filesystem::create_directories(output);
}

std::filesystem::path default_output_for(const std::filesystem::path& input) {
  auto name = input.filename().string();
  if (name.empty()) name = "venom";
  if (std::filesystem::is_regular_file(input)) name = input.stem().string();
  return std::filesystem::absolute(input).parent_path() / (name + "-recovered");
}

const venom::package::Section* find_section(const venom::package::Package& pkg,
                                            venom::package::SectionType type,
                                            const std::string& canonical = {}) {
  for (const auto& section : pkg.sections) {
    if (section.type != type) continue;
    if (canonical.empty() || venom::package::section_name_matches(type, section.name, canonical)) return &section;
  }
  return nullptr;
}

PolyMap parse_poly(const std::vector<unsigned char>& bytes) {
  if (!has_magic(bytes, "VPOL0010")) throw std::runtime_error("invalid VM polymorph section magic");
  if (bytes.size() < 72u) throw std::runtime_error("truncated VM polymorph section");
  if (read_u32(bytes, 8u, "VM polymorph version") != 2u) throw std::runtime_error("unsupported VM polymorph version");
  PolyMap map;
  map.opcode_mask = read_u32(bytes, 20u, "VM opcode mask");
  map.operand_masks[0] = read_u32(bytes, 24u, "VM operand mask A");
  map.operand_masks[1] = read_u32(bytes, 28u, "VM operand mask B");
  map.operand_masks[2] = read_u32(bytes, 32u, "VM operand mask C");
  std::set<unsigned> seen_words;
  for (std::size_t i = 0; i < 4u; ++i) {
    const auto raw = bytes[36u + i];
    if (raw > static_cast<unsigned char>(venom::vm::EncodedWord::C) || !seen_words.insert(raw).second) {
      throw std::runtime_error("invalid VM word layout");
    }
    map.word_layout[i] = static_cast<venom::vm::EncodedWord>(raw);
  }
  const auto count = read_u32(bytes, 48u, "VM opcode map count");
  if (count > 65536u || 72ull + static_cast<std::uint64_t>(count) * 4ull > bytes.size()) {
    throw std::runtime_error("invalid VM opcode map range");
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    const std::size_t offset = 72u + static_cast<std::size_t>(i) * 4u;
    const std::uint32_t logical = static_cast<std::uint32_t>(bytes[offset]) |
                                  (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u);
    const std::uint32_t physical = static_cast<std::uint32_t>(bytes[offset + 2u]) |
                                   (static_cast<std::uint32_t>(bytes[offset + 3u]) << 8u);
    map.physical_to_logical[physical] = logical;
  }
  if (map.physical_to_logical.empty()) throw std::runtime_error("empty VM opcode map");
  return map;
}

std::uint32_t string_stream_next(std::uint32_t& state) {
  state ^= state << 13u;
  state ^= state >> 17u;
  state ^= state << 5u;
  return state;
}

std::vector<std::string> parse_strings(const std::vector<unsigned char>& bytes) {
  if (!has_magic(bytes, "VSTR0011")) throw std::runtime_error("invalid string table magic");
  if (bytes.size() < 20u) throw std::runtime_error("truncated string table");
  if (read_u32(bytes, 8u, "string table version") != 2u) throw std::runtime_error("unsupported string table version");
  const auto count = read_u32(bytes, 12u, "string table count");
  const auto seed = read_u32(bytes, 16u, "string table seed");
  if (count > 1'000'000u) throw std::runtime_error("string table count exceeds recovery limit");
  const std::uint64_t data_base64 = 20ull + static_cast<std::uint64_t>(count) * 12ull;
  if (data_base64 > bytes.size()) throw std::runtime_error("truncated string table entries");
  const auto data_base = static_cast<std::size_t>(data_base64);
  std::vector<std::string> strings;
  strings.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto entry = 20u + static_cast<std::size_t>(i) * 12u;
    const auto offset = read_u32(bytes, entry, "string offset");
    const auto size = read_u32(bytes, entry + 4u, "string size");
    const auto nonce = read_u32(bytes, entry + 8u, "string nonce");
    if (offset > bytes.size() - data_base || size > bytes.size() - data_base - offset) {
      throw std::runtime_error("string table data range is invalid");
    }
    std::string value(size, '\0');
    std::uint32_t state = seed ^ nonce ^ (size * 0x27D4EB2Du);
    for (std::uint32_t j = 0; j < size; ++j) {
      value[j] = static_cast<char>(bytes[data_base + offset + j] ^ static_cast<unsigned char>(string_stream_next(state) & 0xffu));
    }
    strings.push_back(std::move(value));
  }
  return strings;
}

std::vector<RouteEntry> parse_routes(const std::vector<unsigned char>& bytes) {
  if (!has_magic(bytes, "VRTE0003")) throw std::runtime_error("invalid route table magic");
  if (bytes.size() < 16u) throw std::runtime_error("truncated route table");
  const auto version = read_u32(bytes, 8u, "route table version");
  if (version != 1u && version != 2u) throw std::runtime_error("unsupported route table version");
  const auto count = read_u32(bytes, 12u, "route count");
  if (16ull + static_cast<std::uint64_t>(count) * 24ull > bytes.size()) throw std::runtime_error("truncated route entries");
  std::vector<RouteEntry> routes;
  routes.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto offset = 16u + static_cast<std::size_t>(i) * 24u;
    routes.push_back({
        read_u32(bytes, offset + 0u, "route id"),
        read_u32(bytes, offset + 4u, "source id"),
        read_u32(bytes, offset + 8u, "route bytecode offset"),
        read_u32(bytes, offset + 12u, "route bytecode size"),
        read_u32(bytes, offset + 16u, "route instruction count"),
        read_u32(bytes, offset + 20u, "route flags")});
  }
  return routes;
}

VmImage parse_vm(const std::vector<unsigned char>& bytes) {
  if (!has_magic(bytes, "VBCODE03")) throw std::runtime_error("invalid VM bytecode magic");
  if (bytes.size() < 24u) throw std::runtime_error("truncated VM bytecode");
  if (read_u32(bytes, 8u, "VM bytecode version") != 1u) throw std::runtime_error("unsupported VM bytecode version");
  VmImage image;
  image.instruction_size = read_u32(bytes, 12u, "VM instruction size");
  image.instruction_count = read_u32(bytes, 16u, "VM instruction count");
  image.flags = read_u32(bytes, 20u, "VM flags");
  if (image.instruction_size != 16u) throw std::runtime_error("unsupported VM instruction size");
  image.data = slice(bytes, 24u, bytes.size() - 24u, "VM instruction stream");
  if (image.data.size() % 16u != 0u) throw std::runtime_error("VM instruction stream is not aligned");
  return image;
}

DecodedInstruction decode_instruction(const VmImage& vm, std::size_t offset, const PolyMap& poly) {
  if (offset > vm.data.size() || vm.data.size() - offset < 16u) throw std::runtime_error("truncated VM instruction");
  std::array<std::uint32_t, 4> words{};
  for (std::size_t i = 0; i < 4u; ++i) words[i] = read_u32(vm.data, offset + i * 4u, "VM word");
  std::uint32_t physical = 0, a = 0, b = 0, c = 0;
  for (std::size_t i = 0; i < 4u; ++i) {
    switch (poly.word_layout[i]) {
      case venom::vm::EncodedWord::Opcode: physical = words[i] ^ poly.opcode_mask; break;
      case venom::vm::EncodedWord::A: a = words[i] ^ poly.operand_masks[0]; break;
      case venom::vm::EncodedWord::B: b = words[i] ^ poly.operand_masks[1]; break;
      case venom::vm::EncodedWord::C: c = words[i] ^ poly.operand_masks[2]; break;
    }
  }
  const auto found = poly.physical_to_logical.find(physical);
  if (found == poly.physical_to_logical.end() || found->second > static_cast<std::uint32_t>(venom::vm::LogicalOpcode::Halt)) {
    throw std::runtime_error("unknown physical VM opcode " + std::to_string(physical));
  }
  return {static_cast<venom::vm::LogicalOpcode>(found->second), a, b, c};
}

std::string string_ref(const std::vector<std::string>& strings, std::uint32_t id) {
  if (id >= strings.size()) return "<invalid-string:" + std::to_string(id) + ">";
  return strings[id];
}

std::string escape_html(const std::string& value, bool attribute) {
  std::string out;
  for (char c : value) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += attribute ? "&quot;" : "\""; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

bool void_element(const std::string& tag) {
  static const std::set<std::string> tags = {"area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr"};
  return tags.count(lower(tag)) != 0u;
}

void render_node(const HtmlNode& node, std::ostringstream& out, std::size_t depth) {
  const std::string indent(depth * 2u, ' ');
  if (node.tag.empty()) {
    for (const auto& item : node.items) {
      if (item.child) render_node(*item.child, out, depth);
      else out << indent << escape_html(item.text, false) << '\n';
    }
    return;
  }
  out << indent << '<' << node.tag;
  for (const auto& [name, value] : node.attributes) out << ' ' << name << "=\"" << escape_html(value, true) << '"';
  if (void_element(node.tag)) {
    out << ">\n";
    return;
  }
  if (node.items.empty()) {
    out << "></" << node.tag << ">\n";
    return;
  }
  const bool text_only = std::all_of(node.items.begin(), node.items.end(), [](const HtmlItem& item) { return !item.child; });
  if (text_only) {
    out << '>';
    for (const auto& item : node.items) out << escape_html(item.text, false);
    out << "</" << node.tag << ">\n";
    return;
  }
  out << ">\n";
  for (const auto& item : node.items) {
    if (item.child) render_node(*item.child, out, depth + 1u);
    else if (!item.text.empty()) out << std::string((depth + 1u) * 2u, ' ') << escape_html(item.text, false) << '\n';
  }
  out << indent << "</" << node.tag << ">\n";
}

std::string rebuild_html(const std::vector<DecodedInstruction>& instructions,
                         const std::vector<std::string>& strings,
                         std::vector<std::string>& warnings) {
  auto root = std::make_unique<HtmlNode>();
  std::vector<std::unique_ptr<HtmlNode>> frames;
  frames.push_back(std::move(root));
  std::unique_ptr<HtmlNode> created;
  std::unique_ptr<HtmlNode> completed;
  for (const auto& ins : instructions) {
    switch (ins.opcode) {
      case venom::vm::LogicalOpcode::CreateElement:
        created = std::make_unique<HtmlNode>();
        created->tag = string_ref(strings, ins.a);
        break;
      case venom::vm::LogicalOpcode::EnterElement:
        if (!created) { warnings.push_back("enter_element without create_element"); break; }
        frames.push_back(std::move(created));
        break;
      case venom::vm::LogicalOpcode::SetAttribute:
        if (frames.empty()) { warnings.push_back("set_attribute without active element"); break; }
        frames.back()->attributes.emplace_back(string_ref(strings, ins.a), string_ref(strings, ins.b));
        break;
      case venom::vm::LogicalOpcode::SetText:
        if (frames.empty()) { warnings.push_back("set_text without active element"); break; }
        frames.back()->items.push_back({string_ref(strings, ins.a), nullptr});
        break;
      case venom::vm::LogicalOpcode::LeaveElement:
        if (frames.size() <= 1u) { warnings.push_back("leave_element underflow"); break; }
        completed = std::move(frames.back());
        frames.pop_back();
        break;
      case venom::vm::LogicalOpcode::AppendChild:
        if (!completed || frames.empty()) { warnings.push_back("append_child without completed element"); break; }
        frames.back()->items.push_back({{}, std::move(completed)});
        break;
      case venom::vm::LogicalOpcode::CallQuickJs:
        if (!frames.empty()) frames.back()->items.push_back({"", nullptr});
        break;
      case venom::vm::LogicalOpcode::Halt:
      case venom::vm::LogicalOpcode::Nop:
      case venom::vm::LogicalOpcode::LoadConst:
        break;
    }
  }
  while (frames.size() > 1u) {
    auto child = std::move(frames.back());
    frames.pop_back();
    frames.back()->items.push_back({{}, std::move(child)});
  }
  std::ostringstream out;
  out << "<!-- Structurally reconstructed from Venom Route VM bytecode. -->\n";
  render_node(*frames.front(), out, 0u);
  return out.str();
}

std::string disassemble_route(const std::string& route,
                              const std::string& source,
                              const std::vector<DecodedInstruction>& instructions,
                              const std::vector<std::string>& strings) {
  std::ostringstream out;
  out << "VENOM_ROUTE_VM_DISASSEMBLY_V1\n"
      << "route=" << route << "\nsource=" << source << "\ninstructions=" << instructions.size() << "\n\n";
  for (std::size_t i = 0; i < instructions.size(); ++i) {
    const auto& ins = instructions[i];
    out << std::right << std::setw(6) << std::setfill('0') << i
        << std::setfill(' ') << "  " << std::left << std::setw(18) << venom::vm::opcode_name(ins.opcode)
        << " a=" << ins.a << " b=" << ins.b << " c=" << ins.c;
    switch (ins.opcode) {
      case venom::vm::LogicalOpcode::CreateElement:
      case venom::vm::LogicalOpcode::SetText:
        out << "  ; \"" << json_escape(string_ref(strings, ins.a)) << "\"";
        break;
      case venom::vm::LogicalOpcode::SetAttribute:
        out << "  ; \"" << json_escape(string_ref(strings, ins.a)) << "\" = \"" << json_escape(string_ref(strings, ins.b)) << "\"";
        break;
      default: break;
    }
    out << '\n';
  }
  return out.str();
}

std::uint32_t xorshift32(std::uint32_t& state) {
  state ^= state << 13u;
  state ^= state >> 17u;
  state ^= state << 5u;
  return state;
}

std::array<unsigned char, 16> lane_map(std::uint32_t seed) {
  std::array<unsigned char, 16> map{};
  for (unsigned char i = 0; i < map.size(); ++i) map[i] = i;
  std::uint32_t state = seed ^ 0xC6EF3720u;
  for (std::size_t i = map.size() - 1u; i > 0u; --i) {
    const std::size_t j = xorshift32(state) % (i + 1u);
    std::swap(map[i], map[j]);
  }
  return map;
}

std::uint32_t lane_fingerprint(const std::array<unsigned char, 16>& map) {
  std::uint32_t h = 2166136261u;
  for (unsigned char value : map) { h ^= value; h *= 16777619u; }
  return h;
}

std::vector<unsigned char> unwrap_envelope(const std::vector<unsigned char>& bytes) {
  if (!has_magic(bytes, "VQJSE006")) return bytes;
  if (bytes.size() < 48u) throw std::runtime_error("truncated QuickJS envelope");
  const auto version = read_u32(bytes, 8u, "QuickJS envelope version");
  const auto raw_size = read_u32(bytes, 12u, "QuickJS envelope raw size");
  const auto seed = read_u32(bytes, 16u, "QuickJS envelope seed");
  const auto abi = read_u32(bytes, 20u, "QuickJS envelope ABI");
  const auto raw_hash = read_u64(bytes, 24u, "QuickJS envelope hash");
  const auto payload_offset = read_u32(bytes, 32u, "QuickJS envelope payload offset");
  const auto lane_width = read_u32(bytes, 40u, "QuickJS envelope lane width");
  const auto expected_fingerprint = read_u32(bytes, 44u, "QuickJS envelope lane fingerprint");
  if (version != 2u || abi != 0x01000300u || payload_offset != 48u || lane_width != 16u) {
    throw std::runtime_error("unsupported QuickJS envelope contract");
  }
  if (raw_size > bytes.size() - payload_offset) throw std::runtime_error("truncated QuickJS envelope payload");
  const auto map = lane_map(seed);
  if (lane_fingerprint(map) != expected_fingerprint) throw std::runtime_error("QuickJS envelope lane fingerprint mismatch");
  std::vector<unsigned char> raw(raw_size);
  std::uint32_t stream = seed ^ 0x9E3779B9u;
  for (std::size_t block = 0; block < raw.size(); block += 16u) {
    const auto block_size = std::min<std::size_t>(16u, raw.size() - block);
    std::array<unsigned char, 16> block_map{};
    std::size_t block_map_size = 0;
    for (const auto lane : map) if (lane < block_size) block_map[block_map_size++] = lane;
    for (std::size_t stored_lane = 0; stored_lane < block_size; ++stored_lane) {
      if (((block + stored_lane) & 3u) == 0u) xorshift32(stream);
      const auto mask = static_cast<unsigned char>((stream >> (((block + stored_lane) & 3u) * 8u)) & 0xffu);
      raw[block + block_map[stored_lane]] = static_cast<unsigned char>(bytes[payload_offset + block + stored_lane] ^ mask);
    }
  }
  if (venom::package::fnv1a64(raw) != raw_hash) throw std::runtime_error("QuickJS envelope integrity mismatch");
  return raw;
}

QuickJsRecord parse_quickjs_record(const std::vector<unsigned char>& bytes) {
  if (!has_magic(bytes, "VQJSBC03")) throw std::runtime_error("invalid QuickJS record magic");
  if (bytes.size() < 48u) throw std::runtime_error("truncated QuickJS record");
  if (read_u32(bytes, 8u, "QuickJS record version") != 3u) throw std::runtime_error("unsupported QuickJS record version");
  const auto flags = read_u32(bytes, 12u, "QuickJS record flags");
  const auto payload_size = read_u32(bytes, 16u, "QuickJS record payload size");
  const auto abi = read_u32(bytes, 20u, "QuickJS record ABI");
  const auto bytecode_hash = read_u64(bytes, 24u, "QuickJS record bytecode hash");
  const auto source_hash = read_u64(bytes, 32u, "QuickJS record source hash");
  const auto source_size = read_u32(bytes, 40u, "QuickJS record source size");
  const auto payload_offset = read_u32(bytes, 44u, "QuickJS record payload offset");
  if (payload_offset != 48u || payload_size > bytes.size() - payload_offset) throw std::runtime_error("invalid QuickJS record payload range");
  auto payload = slice(bytes, payload_offset, payload_size, "QuickJS object bytecode");
  if (venom::package::fnv1a64(payload) != bytecode_hash) throw std::runtime_error("QuickJS object hash mismatch");
  return {(flags & 1u) != 0u, abi, bytecode_hash, source_hash, source_size, std::move(payload)};
}

std::vector<ScriptChunk> parse_script_bundle(const std::vector<unsigned char>& bytes) {
  if (!has_magic(bytes, "VJSB0006")) throw std::runtime_error("invalid script bundle magic");
  if (bytes.size() < 24u) throw std::runtime_error("truncated script bundle");
  if (read_u32(bytes, 8u, "script bundle version") != 1u) throw std::runtime_error("unsupported script bundle version");
  const auto count = read_u32(bytes, 12u, "script bundle count");
  const auto text_size = read_u32(bytes, 16u, "script bundle text size");
  const std::uint64_t text_base64 = 24ull + static_cast<std::uint64_t>(count) * 40ull;
  if (text_base64 > bytes.size() || text_size > bytes.size() - text_base64) throw std::runtime_error("invalid script bundle table range");
  const auto text_base = static_cast<std::size_t>(text_base64);
  const auto code_base = text_base + text_size;
  std::vector<ScriptChunk> chunks;
  chunks.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto entry = 24u + static_cast<std::size_t>(i) * 40u;
    const auto route_offset = read_u32(bytes, entry + 0u, "script route offset");
    const auto route_size = read_u32(bytes, entry + 4u, "script route size");
    const auto source_offset = read_u32(bytes, entry + 8u, "script source offset");
    const auto source_size = read_u32(bytes, entry + 12u, "script source size");
    const auto code_offset = read_u32(bytes, entry + 16u, "script code offset");
    const auto code_size = read_u32(bytes, entry + 20u, "script code size");
    const auto order = read_u32(bytes, entry + 24u, "script order");
    const auto flags = read_u32(bytes, entry + 28u, "script flags");
    const auto kind = read_u32(bytes, entry + 32u, "script kind");
    if (route_offset > text_size || route_size > text_size - route_offset ||
        source_offset > text_size || source_size > text_size - source_offset ||
        code_offset > bytes.size() - code_base || code_size > bytes.size() - code_base - code_offset) {
      throw std::runtime_error("invalid script bundle entry range");
    }
    chunks.push_back({
        read_string(bytes, text_base + route_offset, route_size, "script route"),
        read_string(bytes, text_base + source_offset, source_size, "script source"),
        order,
        flags,
        kind,
        slice(bytes, code_base + code_offset, code_size, "script code")});
  }
  return chunks;
}

std::vector<std::string> collect_printable_strings(const std::vector<unsigned char>& bytes, std::size_t minimum = 4u) {
  std::set<std::string> unique;
  std::string current;
  for (unsigned char c : bytes) {
    if (c >= 0x20u && c < 0x7fu) current.push_back(static_cast<char>(c));
    else {
      if (current.size() >= minimum) unique.insert(current);
      current.clear();
    }
  }
  if (current.size() >= minimum) unique.insert(current);
  return {unique.begin(), unique.end()};
}

std::string join_lines(const std::vector<std::string>& values) {
  std::ostringstream out;
  for (const auto& value : values) out << value << '\n';
  return out.str();
}

bool is_probable_identifier(const std::string& value) {
  if (value.empty() || value.size() > 96u) return false;
  const auto first = static_cast<unsigned char>(value.front());
  if (!(std::isalpha(first) != 0 || value.front() == '_' || value.front() == '$')) return false;
  for (const char c : value) {
    const auto u = static_cast<unsigned char>(c);
    if (!(std::isalnum(u) != 0 || c == '_' || c == '$')) return false;
  }
  static const std::set<std::string> noise = {"true", "false", "null", "undefined", "object", "string", "number", "function"};
  return noise.find(value) == noise.end();
}

bool is_probable_url(const std::string& value) {
  const auto v = lower(value);
  return v.rfind("http://", 0u) == 0u || v.rfind("https://", 0u) == 0u || v.rfind("ws://", 0u) == 0u ||
         v.rfind("wss://", 0u) == 0u || v.rfind("data:", 0u) == 0u;
}

bool is_probable_path(const std::string& value) {
  if (value.size() < 3u || value.size() > 240u || is_probable_url(value)) return false;
  return value.find('/') != std::string::npos || value.find("\\") != std::string::npos ||
         value.rfind("./", 0u) == 0u || value.rfind("../", 0u) == 0u;
}

double shannon_entropy(const std::vector<unsigned char>& bytes) {
  if (bytes.empty()) return 0.0;
  std::array<std::size_t, 256> counts{};
  for (const auto byte : bytes) ++counts[byte];
  double entropy = 0.0;
  for (const auto count : counts) {
    if (!count) continue;
    const double p = static_cast<double>(count) / static_cast<double>(bytes.size());
    entropy -= p * std::log2(p);
  }
  return entropy;
}

std::string json_string_array(const std::vector<std::string>& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) out << ',';
    out << "\n    \"" << json_escape(values[i]) << "\"";
  }
  if (!values.empty()) out << '\n';
  out << "  ]";
  return out.str();
}

std::string beautify_javascript(const std::string& source) {
  std::string input = source;
  const auto marker = input.rfind(";void\"vbind:");
  if (marker != std::string::npos && input.find("\";", marker) != std::string::npos) input.erase(marker);

  enum class State { Normal, Single, Double, Template, Regex, LineComment, BlockComment };
  State state = State::Normal;
  std::string out;
  out.reserve(input.size() + input.size() / 8u + 64u);
  std::size_t indent = 0;
  std::size_t paren_depth = 0;
  bool escaped = false;
  bool regex_class = false;
  bool line_start = true;
  char previous_significant = 0;

  auto emit_indent = [&]() {
    if (line_start) {
      out.append(indent * 2u, ' ');
      line_start = false;
    }
  };
  auto newline = [&]() {
    while (!out.empty() && (out.back() == ' ' || out.back() == '\t')) out.pop_back();
    if (out.empty() || out.back() != '\n') out.push_back('\n');
    line_start = true;
  };
  const auto regex_allowed_after = [](char c) {
    return c == 0 || c == '(' || c == '[' || c == '{' || c == '=' || c == ':' || c == ',' || c == ';' ||
           c == '!' || c == '?' || c == '&' || c == '|' || c == '+' || c == '-' || c == '*' || c == '%' || c == '~';
  };

  for (std::size_t i = 0; i < input.size(); ++i) {
    const char c = input[i];
    const char next = i + 1u < input.size() ? input[i + 1u] : 0;
    if (state == State::LineComment) {
      emit_indent();
      out.push_back(c);
      if (c == '\n') { state = State::Normal; line_start = true; }
      continue;
    }
    if (state == State::BlockComment) {
      emit_indent();
      out.push_back(c);
      if (c == '*' && next == '/') { out.push_back(next); ++i; state = State::Normal; }
      continue;
    }
    if (state == State::Single || state == State::Double || state == State::Template) {
      emit_indent();
      out.push_back(c);
      if (escaped) { escaped = false; continue; }
      if (c == '\\') { escaped = true; continue; }
      if ((state == State::Single && c == '\'') || (state == State::Double && c == '"') || (state == State::Template && c == '`')) state = State::Normal;
      continue;
    }
    if (state == State::Regex) {
      emit_indent();
      out.push_back(c);
      if (escaped) { escaped = false; continue; }
      if (c == '\\') { escaped = true; continue; }
      if (c == '[') regex_class = true;
      else if (c == ']') regex_class = false;
      else if (c == '/' && !regex_class) {
        while (i + 1u < input.size() && std::isalpha(static_cast<unsigned char>(input[i + 1u])) != 0) out.push_back(input[++i]);
        state = State::Normal;
        previous_significant = '/';
      }
      continue;
    }

    if (c == '/' && next == '/') { emit_indent(); out += "//"; ++i; state = State::LineComment; continue; }
    if (c == '/' && next == '*') { emit_indent(); out += "/*"; ++i; state = State::BlockComment; continue; }
    if (c == '/' && regex_allowed_after(previous_significant)) { emit_indent(); out.push_back(c); state = State::Regex; regex_class = false; continue; }
    if (c == '\'' || c == '"' || c == '`') {
      emit_indent();
      out.push_back(c);
      state = c == '\'' ? State::Single : (c == '"' ? State::Double : State::Template);
      previous_significant = c;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(c)) != 0) {
      if (!line_start && next && std::isspace(static_cast<unsigned char>(next)) == 0 && (out.empty() || out.back() != ' ')) out.push_back(' ');
      continue;
    }
    if (c == '(') { emit_indent(); out.push_back(c); ++paren_depth; previous_significant = c; continue; }
    if (c == ')') { emit_indent(); out.push_back(c); if (paren_depth) --paren_depth; previous_significant = c; continue; }
    if (c == '{') {
      emit_indent();
      if (!out.empty() && out.back() != ' ' && out.back() != '\n') out.push_back(' ');
      out.push_back('{');
      ++indent;
      newline();
      previous_significant = c;
      continue;
    }
    if (c == '}') {
      if (!line_start) newline();
      if (indent) --indent;
      emit_indent();
      out.push_back(c);
      const char after = next;
      if (after != ';' && after != ',' && after != ')' && after != ']' && after != '.') newline();
      previous_significant = c;
      continue;
    }
    if (c == ';') {
      emit_indent();
      out.push_back(c);
      if (paren_depth == 0u) newline();
      else out.push_back(' ');
      previous_significant = c;
      continue;
    }
    emit_indent();
    out.push_back(c);
    previous_significant = c;
  }
  while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back())) != 0) out.pop_back();
  out.push_back('\n');
  return out;
}

#if defined(VENOM_ENABLE_FULL_QUICKJS)
bool dump_quickjs_object(const std::vector<unsigned char>& payload,
                         const std::filesystem::path& path,
                         std::string& error) {
  std::filesystem::create_directories(path.parent_path());
  std::fflush(stdout);
#if defined(_WIN32)
  const int saved = _dup(_fileno(stdout));
#else
  const int saved = dup(fileno(stdout));
#endif
  if (saved < 0) { error = "failed to duplicate stdout"; return false; }
  FILE* redirected = std::freopen(path.string().c_str(), "wb", stdout);
  if (!redirected) {
#if defined(_WIN32)
    _dup2(saved, 1);
    _close(saved);
#else
    dup2(saved, STDOUT_FILENO);
    close(saved);
#endif
    std::clearerr(stdout);
    error = "failed to redirect QuickJS dump output";
    return false;
  }
  bool ok = false;
  JSRuntime* runtime = JS_NewRuntime();
  if (runtime) {
    JS_SetMemoryLimit(runtime, 128u * 1024u * 1024u);
    JS_SetMaxStackSize(runtime, 4u * 1024u * 1024u);
  }
  JSContext* context = runtime ? JS_NewContext(runtime) : nullptr;
  if (runtime && context) {
    JS_SetDumpFlags(runtime, JS_DUMP_READ_OBJECT);
    JSValue object = JS_ReadObject(context, payload.data(), payload.size(), JS_READ_OBJ_BYTECODE | JS_READ_OBJ_REFERENCE);
    ok = !JS_IsException(object);
    if (!ok) {
      JSValue exception = JS_GetException(context);
      const char* text = JS_ToCString(context, exception);
      error = text ? text : "QuickJS rejected the bytecode object";
      if (text) JS_FreeCString(context, text);
      JS_FreeValue(context, exception);
    }
    JS_FreeValue(context, object);
  } else {
    error = "failed to create QuickJS dump context";
  }
  if (context) JS_FreeContext(context);
  if (runtime) JS_FreeRuntime(runtime);
  std::fflush(stdout);
#if defined(_WIN32)
  _dup2(saved, 1);
  _close(saved);
#else
  dup2(saved, STDOUT_FILENO);
  close(saved);
#endif
  std::clearerr(stdout);
  return ok;
}
#else
bool dump_quickjs_object(const std::vector<unsigned char>&,
                         const std::filesystem::path&,
                         std::string& error) {
  error = "QuickJS disassembly requires VENOM_ENABLE_FULL_QUICKJS=ON";
  return false;
}
#endif

void write_quickjs_record(const QuickJsRecord& record,
                          const std::filesystem::path& directory,
                          const std::string& label,
                          const Options& options,
                          RecoveryStats& stats) {
  std::filesystem::create_directories(directory);
  write_bytes(directory / "quickjs-object.bin", record.payload);
  const auto strings = collect_printable_strings(record.payload);
  std::vector<std::string> identifiers;
  std::vector<std::string> urls;
  std::vector<std::string> paths;
  for (const auto& value : strings) {
    if (is_probable_identifier(value)) identifiers.push_back(value);
    if (is_probable_url(value)) urls.push_back(value);
    else if (is_probable_path(value)) paths.push_back(value);
  }
  write_text(directory / "printable-strings.txt", join_lines(strings));
  write_text(directory / "probable-identifiers.txt", join_lines(identifiers));
  write_text(directory / "urls.txt", join_lines(urls));
  write_text(directory / "module-paths.txt", join_lines(paths));
  stats.quickjs_strings += strings.size();
  stats.quickjs_identifiers += identifiers.size();
  stats.quickjs_urls += urls.size();
  stats.quickjs_paths += paths.size();
  std::ostringstream analysis;
  analysis << "{\n"
           << "  \"entropyBitsPerByte\": " << std::fixed << std::setprecision(4) << shannon_entropy(record.payload) << ",\n"
           << "  \"printableStringCount\": " << strings.size() << ",\n"
           << "  \"probableIdentifierCount\": " << identifiers.size() << ",\n"
           << "  \"urlCount\": " << urls.size() << ",\n"
           << "  \"modulePathCount\": " << paths.size() << ",\n"
           << "  \"probableIdentifiers\": " << json_string_array(identifiers) << ",\n"
           << "  \"urls\": " << json_string_array(urls) << ",\n"
           << "  \"modulePaths\": " << json_string_array(paths) << "\n"
           << "}\n";
  write_text(directory / "static-analysis.json", analysis.str());
  std::ostringstream metadata;
  metadata << "{\n"
           << "  \"label\": \"" << json_escape(label) << "\",\n"
           << "  \"module\": " << (record.module ? "true" : "false") << ",\n"
           << "  \"abi\": " << record.abi << ",\n"
           << "  \"bytecodeBytes\": " << record.payload.size() << ",\n"
           << "  \"bytecodeHash\": \"" << hex64(record.bytecode_hash) << "\",\n"
           << "  \"sourceBytes\": " << record.source_size << ",\n"
           << "  \"sourceHash\": \"" << hex64(record.source_hash) << "\",\n"
           << "  \"sourcePresent\": false\n"
           << "}\n";
  write_text(directory / "record.json", metadata.str());
  write_text(directory / "SOURCE-NOT-RECOVERABLE.txt",
             "This record contains stripped native QuickJS object bytecode.\n"
             "The exact original JavaScript source, comments, formatting, source maps, and most source-level names are not present.\n"
             "Use quickjs-read-object.txt and printable-strings.txt for structural analysis and reconstruct equivalent logic from behavior.\n");
  if (options.quickjs_disassembly) {
    std::string error;
    if (dump_quickjs_object(record.payload, directory / "quickjs-read-object.txt", error)) ++stats.quickjs_disassemblies;
    else stats.warnings.push_back("QuickJS dump unavailable for " + label + ": " + error);
  }
  ++stats.quickjs_records;
}

void recover_quickjs_value(const std::vector<unsigned char>& stored,
                           const std::filesystem::path& directory,
                           const std::string& label,
                           const Options& options,
                           RecoveryStats& stats) {
  const auto raw = unwrap_envelope(stored);
  write_bytes(directory / "decoded-record.bin", raw);
  if (has_magic(raw, "VQJSBC03")) {
    write_quickjs_record(parse_quickjs_record(raw), directory, label, options, stats);
    return;
  }
  if (has_magic(raw, "VQJSMB04")) {
    if (raw.size() < 24u) throw std::runtime_error("truncated QuickJS module bundle");
    if (read_u32(raw, 8u, "QuickJS module bundle version") != 1u) throw std::runtime_error("unsupported QuickJS module bundle version");
    const auto count = read_u32(raw, 12u, "QuickJS module count");
    const auto entry_index = read_u32(raw, 16u, "QuickJS module entry index");
    const auto table_offset = read_u32(raw, 20u, "QuickJS module table offset");
    if (entry_index >= count || table_offset != 24u || 24ull + static_cast<std::uint64_t>(count) * 16ull > raw.size()) {
      throw std::runtime_error("invalid QuickJS module bundle table");
    }
    std::ostringstream bundle_meta;
    bundle_meta << "{\n  \"entryIndex\": " << entry_index << ",\n  \"moduleCount\": " << count << ",\n  \"modules\": [\n";
    for (std::uint32_t i = 0; i < count; ++i) {
      const auto entry = 24u + static_cast<std::size_t>(i) * 16u;
      const auto name_offset = read_u32(raw, entry + 0u, "QuickJS module name offset");
      const auto name_size = read_u32(raw, entry + 4u, "QuickJS module name size");
      const auto record_offset = read_u32(raw, entry + 8u, "QuickJS module record offset");
      const auto record_size = read_u32(raw, entry + 12u, "QuickJS module record size");
      const auto name = read_string(raw, name_offset, name_size, "QuickJS module name");
      const auto record_bytes = slice(raw, record_offset, record_size, "QuickJS module record");
      const auto module_dir = directory / ("module-" + std::to_string(i) + "-" + sanitize_component(name));
      write_quickjs_record(parse_quickjs_record(record_bytes), module_dir, name, options, stats);
      ++stats.quickjs_modules;
      bundle_meta << "    {\"index\": " << i << ", \"entry\": " << (i == entry_index ? "true" : "false")
                  << ", \"name\": \"" << json_escape(name) << "\"}" << (i + 1u == count ? "" : ",") << "\n";
    }
    bundle_meta << "  ]\n}\n";
    write_text(directory / "module-bundle.json", bundle_meta.str());
    return;
  }
  stats.warnings.push_back("unknown protected script payload format in " + label);
  write_text(directory / "unknown-format.txt", "Decoded payload does not use VQJSBC03 or VQJSMB04.\n");
}

void recover_script_bundle(const venom::package::Section& section,
                           const std::filesystem::path& output,
                           const Options& options,
                           RecoveryStats& stats) {
  const auto chunks = parse_script_bundle(section.data);
  const auto bundle_dir = output / "scripts" / ("bundle-" + std::to_string(stats.script_bundles) + "-" + sanitize_component(section.name));
  std::filesystem::create_directories(bundle_dir);
  std::ostringstream bundle_index;
  bundle_index << "{\n  \"section\": \"" << json_escape(section.name) << "\",\n  \"chunks\": [\n";
  for (std::size_t i = 0; i < chunks.size(); ++i) {
    const auto& chunk = chunks[i];
    const auto chunk_name = "chunk-" + std::to_string(i) + "-" + sanitize_component(chunk.source);
    const auto chunk_dir = bundle_dir / chunk_name;
    std::filesystem::create_directories(chunk_dir);
    std::ostringstream metadata;
    metadata << "{\n"
             << "  \"route\": \"" << json_escape(chunk.route) << "\",\n"
             << "  \"source\": \"" << json_escape(chunk.source) << "\",\n"
             << "  \"order\": " << chunk.order << ",\n"
             << "  \"flags\": " << chunk.flags << ",\n"
             << "  \"kind\": " << chunk.kind << ",\n"
             << "  \"storedBytes\": " << chunk.code.size() << "\n"
             << "}\n";
    write_text(chunk_dir / "chunk.json", metadata.str());
    write_bytes(chunk_dir / "stored-code.bin", chunk.code);
    const bool protected_record = has_magic(chunk.code, "VQJSE006") || has_magic(chunk.code, "VQJSBC03") || has_magic(chunk.code, "VQJSMB04");
    if (protected_record) {
      recover_quickjs_value(chunk.code, chunk_dir, chunk.source, options, stats);
    } else if (looks_textual(chunk.code)) {
      const auto source = bytes_to_text(chunk.code);
      write_text(chunk_dir / "browser-source.original.js", source);
      write_text(chunk_dir / "browser-source.readable.js", beautify_javascript(source));
      ++stats.browser_sources;
    } else {
      stats.warnings.push_back("non-text browser script payload in " + chunk.source);
    }
    bundle_index << "    {\"index\": " << i << ", \"route\": \"" << json_escape(chunk.route)
                 << "\", \"source\": \"" << json_escape(chunk.source) << "\", \"protected\": "
                 << (protected_record ? "true" : "false") << "}" << (i + 1u == chunks.size() ? "" : ",") << "\n";
    ++stats.script_chunks;
  }
  bundle_index << "  ]\n}\n";
  write_text(bundle_dir / "bundle.json", bundle_index.str());
  ++stats.script_bundles;
}

void recover_vm(const venom::package::Package& pkg,
                const std::filesystem::path& output,
                RecoveryStats& stats) {
  const auto* strings_section = find_section(pkg, venom::package::SectionType::Strings, "strings.vstr");
  const auto* routes_section = find_section(pkg, venom::package::SectionType::Routes, "routes.vbrt");
  const auto* vm_section = find_section(pkg, venom::package::SectionType::VmBytecode, "route-bytecode.vmbc");
  const auto* poly_section = find_section(pkg, venom::package::SectionType::Integrity, "vm-polymorph.vpol");
  if (!strings_section || !routes_section || !vm_section || !poly_section) {
    stats.warnings.push_back("Route VM recovery skipped because one or more core sections are missing");
    return;
  }
  const auto strings = parse_strings(strings_section->data);
  const auto routes = parse_routes(routes_section->data);
  const auto vm = parse_vm(vm_section->data);
  const auto poly = parse_poly(poly_section->data);
  const auto vm_dir = output / "route-vm";
  std::filesystem::create_directories(vm_dir);

  std::ostringstream string_dump;
  for (std::size_t i = 0; i < strings.size(); ++i) string_dump << i << '\t' << json_escape(strings[i]) << '\n';
  write_text(vm_dir / "strings.txt", string_dump.str());

  std::ostringstream route_index;
  route_index << "{\n  \"routes\": [\n";
  for (std::size_t route_index_value = 0; route_index_value < routes.size(); ++route_index_value) {
    const auto& route = routes[route_index_value];
    if (route.route_id >= strings.size() || route.source_id >= strings.size()) throw std::runtime_error("route references invalid string id");
    if (route.bytecode_offset > vm.data.size() || route.bytecode_size > vm.data.size() - route.bytecode_offset || route.bytecode_size % 16u != 0u) {
      throw std::runtime_error("route bytecode range is invalid");
    }
    std::vector<DecodedInstruction> instructions;
    for (std::size_t offset = route.bytecode_offset; offset < route.bytecode_offset + route.bytecode_size; offset += 16u) {
      instructions.push_back(decode_instruction(vm, offset, poly));
    }
    const auto& route_name = strings[route.route_id];
    const auto& source_name = strings[route.source_id];
    const auto route_dir = vm_dir / ("route-" + std::to_string(route_index_value) + "-" + sanitize_component(route_name));
    std::filesystem::create_directories(route_dir);
    write_text(route_dir / "disassembly.vasm", disassemble_route(route_name, source_name, instructions, strings));
    std::vector<std::string> html_warnings;
    write_text(route_dir / "reconstructed.html", rebuild_html(instructions, strings, html_warnings));
    if (!html_warnings.empty()) {
      std::ostringstream warning_text;
      for (const auto& warning : html_warnings) warning_text << warning << '\n';
      write_text(route_dir / "reconstruction-warnings.txt", warning_text.str());
      for (const auto& warning : html_warnings) stats.warnings.push_back("route " + route_name + ": " + warning);
    }
    route_index << "    {\"route\": \"" << json_escape(route_name) << "\", \"source\": \"" << json_escape(source_name)
                << "\", \"instructions\": " << instructions.size() << "}" << (route_index_value + 1u == routes.size() ? "" : ",") << "\n";
    ++stats.routes;
    stats.vm_instructions += instructions.size();
  }
  route_index << "  ]\n}\n";
  write_text(vm_dir / "routes.json", route_index.str());
}

void recover_sections(const venom::package::Package& pkg,
                      const std::filesystem::path& output,
                      const Options& options,
                      RecoveryStats& stats) {
  const auto sections_dir = output / "sections";
  std::filesystem::create_directories(sections_dir);
  std::ostringstream index;
  index << "{\n  \"packageVersion\": " << pkg.version << ",\n  \"runtimeAbi\": " << pkg.runtime_abi
        << ",\n  \"packageFlags\": " << pkg.flags << ",\n  \"sections\": [\n";
  for (std::size_t i = 0; i < pkg.sections.size(); ++i) {
    const auto& section = pkg.sections[i];
    const std::string file_name = (i < 10u ? "00" : (i < 100u ? "0" : "")) + std::to_string(i) + "-" +
                                  sanitize_component(venom::package::section_type_name(section.type)) + "-" + sanitize_component(section.name);
    write_bytes(sections_dir / file_name, section.data);
    if (looks_textual(section.data)) write_text(sections_dir / (file_name + ".txt"), bytes_to_text(section.data));
    index << "    {\"index\": " << i
          << ", \"type\": \"" << json_escape(venom::package::section_type_name(section.type))
          << "\", \"name\": \"" << json_escape(section.name)
          << "\", \"decodedBytes\": " << section.data.size()
          << ", \"storedBytes\": " << section.stored_size
          << ", \"compressed\": " << (((section.flags & venom::package::SectionFlagCompressed) != 0u) ? "true" : "false")
          << ", \"encrypted\": " << (((section.flags & venom::package::SectionFlagEncrypted) != 0u) ? "true" : "false")
          << "}" << (i + 1u == pkg.sections.size() ? "" : ",") << "\n";
    if (section.type == venom::package::SectionType::JavaScript && has_magic(section.data, "VJSB0006")) {
      try { recover_script_bundle(section, output, options, stats); }
      catch (const std::exception& ex) { stats.warnings.push_back("script bundle " + section.name + ": " + ex.what()); }
    }
    ++stats.sections;
    ++stats.exact_sections;
  }
  index << "  ]\n}\n";
  write_text(sections_dir / "index.json", index.str());
}

bool path_is_within(const std::filesystem::path& candidate, const std::filesystem::path& root) {
  std::error_code ec;
  const auto relative = std::filesystem::relative(std::filesystem::absolute(candidate), std::filesystem::absolute(root), ec);
  if (ec || relative.empty()) return !ec;
  const auto first = *relative.begin();
  return first != "..";
}

void recover_loose_javascript(const std::filesystem::path& input,
                              const std::filesystem::path& output,
                              RecoveryStats& stats) {
  std::vector<std::filesystem::path> files;
  if (std::filesystem::is_regular_file(input)) {
    const auto ext = lower(input.extension().string());
    if (ext == ".js" || ext == ".mjs" || ext == ".cjs") files.push_back(input);
  } else if (std::filesystem::is_directory(input)) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(input)) {
      if (!entry.is_regular_file()) continue;
      if (path_is_within(entry.path(), output)) continue;
      const auto ext = lower(entry.path().extension().string());
      if (ext == ".js" || ext == ".mjs" || ext == ".cjs") files.push_back(entry.path());
    }
  }
  const auto js_dir = output / "javascript";
  for (const auto& file : files) {
    std::filesystem::path relative = file.filename();
    if (std::filesystem::is_directory(input)) {
      std::error_code ec;
      relative = std::filesystem::relative(file, input, ec);
      if (ec) relative = file.filename();
    }
    std::filesystem::path safe_relative;
    for (const auto& component : relative) safe_relative /= sanitize_component(component.string());
    const auto source = bytes_to_text(read_bytes(file));
    const auto base = js_dir / safe_relative;
    write_text(base.string() + ".original.js", source);
    write_text(base.string() + ".readable.js", beautify_javascript(source));
    ++stats.loose_javascript;
  }
}

void write_report(const RecoveryStats& stats, const Options& options) {
  std::ostringstream json;
  json << "{\n"
       << "  \"schemaVersion\": 1,\n"
       << "  \"input\": \"" << json_escape(stats.input.generic_string()) << "\",\n"
       << "  \"package\": \"" << json_escape(stats.package.generic_string()) << "\",\n"
       << "  \"output\": \"" << json_escape(stats.output.generic_string()) << "\",\n"
       << "  \"recovery\": {\n"
       << "    \"decodedSections\": " << stats.sections << ",\n"
       << "    \"exactDecodedSections\": " << stats.exact_sections << ",\n"
       << "    \"routes\": " << stats.routes << ",\n"
       << "    \"vmInstructions\": " << stats.vm_instructions << ",\n"
       << "    \"scriptBundles\": " << stats.script_bundles << ",\n"
       << "    \"scriptChunks\": " << stats.script_chunks << ",\n"
       << "    \"browserSources\": " << stats.browser_sources << ",\n"
       << "    \"quickJsRecords\": " << stats.quickjs_records << ",\n"
       << "    \"quickJsModules\": " << stats.quickjs_modules << ",\n"
       << "    \"quickJsDisassemblies\": " << stats.quickjs_disassemblies << ",\n"
       << "    \"quickJsPrintableStrings\": " << stats.quickjs_strings << ",\n"
       << "    \"quickJsProbableIdentifiers\": " << stats.quickjs_identifiers << ",\n"
       << "    \"quickJsUrls\": " << stats.quickjs_urls << ",\n"
       << "    \"quickJsModulePaths\": " << stats.quickjs_paths << ",\n"
       << "    \"looseJavaScriptFiles\": " << stats.loose_javascript << "\n"
       << "  },\n"
       << "  \"limits\": {\n"
       << "    \"exactOriginalProtectedSource\": false,\n"
       << "    \"commentsAndFormattingRecoverable\": false,\n"
       << "    \"sourceMapsRecoverable\": false,\n"
       << "    \"bytecodeStructuralAnalysis\": true,\n"
       << "    \"doesNotExecuteRecoveredCode\": true\n"
       << "  },\n"
       << "  \"warnings\": [";
  for (std::size_t i = 0; i < stats.warnings.size(); ++i) {
    if (i) json << ',';
    json << "\n    \"" << json_escape(stats.warnings[i]) << "\"";
  }
  if (!stats.warnings.empty()) json << '\n';
  json << "  ]\n}\n";
  write_text(stats.output / "recovery-report.json", json.str());

  std::ostringstream readme;
  readme << "Venom recovery output\n"
         << "=====================\n\n"
         << "Input: " << stats.input.string() << "\n"
         << "Package: " << (stats.package.empty() ? "none" : stats.package.string()) << "\n\n"
         << "Recovered:\n"
         << "  decoded package sections: " << stats.sections << "\n"
         << "  Route VM routes: " << stats.routes << "\n"
         << "  Route VM instructions: " << stats.vm_instructions << "\n"
         << "  script chunks: " << stats.script_chunks << "\n"
         << "  browser source chunks: " << stats.browser_sources << "\n"
         << "  native QuickJS records: " << stats.quickjs_records << "\n"
         << "  QuickJS modules: " << stats.quickjs_modules << "\n"
         << "  QuickJS read-object dumps: " << stats.quickjs_disassemblies << "\n"
         << "  QuickJS printable strings: " << stats.quickjs_strings << "\n"
         << "  probable identifiers: " << stats.quickjs_identifiers << "\n"
         << "  URLs and module paths: " << (stats.quickjs_urls + stats.quickjs_paths) << "\n"
         << "  loose JavaScript files normalized: " << stats.loose_javascript << "\n\n"
         << "Recovery levels:\n"
         << "  Exact: decoded CSS, assets, metadata, browser-side source, and package sections.\n"
         << "  Structural: Route VM disassembly and reconstructed route HTML.\n"
         << "  Bytecode: unwrapped native QuickJS object records, module bundles, constants, and parser dumps.\n"
         << "  Not recoverable exactly: original protected JavaScript source, comments, source maps, formatting, and stripped identifiers.\n\n"
         << "The tool performs static decoding only. It does not execute recovered protected code.\n";
  write_text(stats.output / "README.txt", readme.str());

  if (options.format == venom::compiler::OutputFormat::Json) {
    std::cout << json.str();
  } else {
    std::cout << "Venom decompilation complete\n"
              << "  input: " << stats.input.string() << "\n"
              << "  package: " << (stats.package.empty() ? "none" : stats.package.string()) << "\n"
              << "  output: " << stats.output.string() << "\n"
              << "  decoded sections: " << stats.sections << "\n"
              << "  reconstructed routes: " << stats.routes << "\n"
              << "  script chunks: " << stats.script_chunks << "\n"
              << "  QuickJS records: " << stats.quickjs_records << "\n"
              << "  QuickJS modules: " << stats.quickjs_modules << "\n"
              << "  normalized JavaScript files: " << (stats.loose_javascript + stats.browser_sources) << "\n"
              << "  warnings: " << stats.warnings.size() << "\n\n"
              << "Exact protected source is not present in a production package; see README.txt and recovery-report.json.\n";
  }
}

std::string require_value(int& i, int argc, char** argv, const std::string& flag) {
  if (i + 1 >= argc) throw std::runtime_error("missing value for " + flag);
  ++i;
  return argv[i];
}

void print_standalone_help() {
  std::cout << "Venom Decompiler\n\n"
            << "Usage:\n"
            << "  venom-decompiler <dist|package|javascript> [--out <directory>] [--key-file <file>]\n"
            << "                       [--format text|json] [--no-js] [--no-qjs-disasm] [--force]\n\n"
            << "Static recovery stages:\n"
            << "  package decode, section extraction, Route VM de-polymorphing, HTML reconstruction,\n"
            << "  VQJSE006 unwrap, VQJSBC03/VQJSMB04 extraction, QuickJS read-object dumps,\n"
            << "  and conservative JavaScript normalization. Recovered code is never executed.\n";
}

} // namespace

bool recover(const Options& requested) {
  Options options = requested;
  if (options.input.empty()) throw std::runtime_error("decompile requires an input path");
  options.input = std::filesystem::absolute(options.input);
  if (!std::filesystem::exists(options.input)) throw std::runtime_error("input does not exist: " + options.input.string());
  if (options.output.empty()) options.output = default_output_for(options.input);
  options.output = std::filesystem::absolute(options.output);
  if (!options.key_file.empty()) venom::compiler::load_package_key_file_for_process(options.key_file);
  prepare_output(options.output, options.force);

  RecoveryStats stats;
  stats.input = options.input;
  stats.output = options.output;
  stats.package = choose_package(options.input);

  bool handled_binary_artifact = false;
  if (!stats.package.empty()) {
    const auto pkg = venom::package::read_package(stats.package);
    recover_sections(pkg, options.output, options, stats);
    try { recover_vm(pkg, options.output, stats); }
    catch (const std::exception& ex) { stats.warnings.push_back(std::string("Route VM recovery: ") + ex.what()); }
  } else if (std::filesystem::is_regular_file(options.input)) {
    const auto bytes = read_bytes(options.input);
    if (has_magic(bytes, "VQJSE006") || has_magic(bytes, "VQJSBC03") || has_magic(bytes, "VQJSMB04")) {
      recover_quickjs_value(bytes,
                            options.output / "quickjs-artifact" / sanitize_component(options.input.filename().string()),
                            options.input.filename().string(),
                            options,
                            stats);
      handled_binary_artifact = true;
    } else if (has_magic(bytes, "VJSB0006")) {
      venom::package::Section section;
      section.type = venom::package::SectionType::JavaScript;
      section.name = options.input.filename().string();
      section.data = bytes;
      section.raw_size = bytes.size();
      section.stored_size = bytes.size();
      recover_script_bundle(section, options.output, options, stats);
      handled_binary_artifact = true;
    }
  }

  const auto input_extension = lower(options.input.extension().string());
  const bool javascript_input = input_extension == ".js" || input_extension == ".mjs" || input_extension == ".cjs";
  if (stats.package.empty() && !handled_binary_artifact && !javascript_input) {
    stats.warnings.push_back("no Venom package, QuickJS record, script bundle, or JavaScript input was recognized");
  }

  if (options.recover_javascript && !handled_binary_artifact) recover_loose_javascript(options.input, options.output, stats);
  write_report(stats, options);
  return true;
}

int run_cli(int argc, char** argv) {
  try {
    if (argc <= 1 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
      print_standalone_help();
      return 0;
    }
    Options options;
    options.input = argv[1];
    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--out") options.output = require_value(i, argc, argv, arg);
      else if (arg == "--key-file") options.key_file = require_value(i, argc, argv, arg);
      else if (arg == "--format") {
        const auto value = require_value(i, argc, argv, arg);
        if (value == "json") options.format = venom::compiler::OutputFormat::Json;
        else if (value != "text") throw std::runtime_error("--format must be text or json");
      } else if (arg == "--no-js") options.recover_javascript = false;
      else if (arg == "--no-qjs-disasm") options.quickjs_disassembly = false;
      else if (arg == "--force") options.force = true;
      else throw std::runtime_error("unknown option: " + arg);
    }
    return recover(options) ? 0 : 1;
  } catch (const std::exception& ex) {
    std::cerr << "venom-decompiler: " << ex.what() << '\n';
    return 10;
  }
}

} // namespace venom::decompiler
