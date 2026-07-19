#include "venom/base/error.hpp"
#include "html.hpp"

#include "venom/core/site.hpp"
#include "venom/vm/encoder.hpp"
#include "venom/vm/opcode.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace venom::compiler {

namespace {

struct Attribute {
  std::string name;
  std::string value;
};

struct Token {
  enum class Kind { StartTag, EndTag, Text } kind = Kind::Text;
  std::string name;
  std::vector<Attribute> attributes;
  std::string text;
  bool self_closing = false;
  bool executable_script = false;
};

void append_u32(std::vector<unsigned char>& out, std::uint32_t value) {
  out.push_back(static_cast<unsigned char>(value & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
}

std::string to_string(const std::vector<unsigned char>& bytes) {
  return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::string lower_ascii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

bool is_name_char(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalnum(u) != 0 || c == '-' || c == '_' || c == ':' || c == '.';
}


std::string trim_ascii_copy(std::string value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) value.erase(value.begin());
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) value.pop_back();
  return value;
}

bool script_is_executable(const std::vector<Attribute>& attributes) {
  std::string type;
  for (const auto& attribute : attributes) {
    if (lower_ascii(attribute.name) == "type") { type = attribute.value; break; }
  }
  type = lower_ascii(trim_ascii_copy(std::move(type)));
  if (const auto semi = type.find(';'); semi != std::string::npos) type.resize(semi);
  type = trim_ascii_copy(std::move(type));
  if (type.empty() || type == "module") return true;
  static const std::vector<std::string> javascript_types = {
    "text/javascript", "application/javascript", "text/ecmascript", "application/ecmascript",
    "application/x-javascript", "text/jscript", "text/livescript"
  };
  return std::find(javascript_types.begin(), javascript_types.end(), type) != javascript_types.end();
}

bool is_void_element(const std::string& name) {
  static const std::vector<std::string> names{
    "area", "base", "br", "col", "embed", "hr", "img", "input", "link",
    "meta", "param", "source", "track", "wbr",
  };
  return std::find(names.begin(), names.end(), lower_ascii(name)) != names.end();
}

void skip_space(std::string_view source, std::size_t& i) {
  while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) {
    ++i;
  }
}

std::string read_name(std::string_view source, std::size_t& i) {
  const auto start = i;
  while (i < source.size() && is_name_char(source[i])) {
    ++i;
  }
  return lower_ascii(std::string(source.substr(start, i - start)));
}

std::string decode_basic_entities(std::string value) {
  std::string out;
  out.reserve(value.size());
  for (std::size_t i = 0; i < value.size();) {
    if (value[i] != '&') { out.push_back(value[i++]); continue; }
    const auto semi = value.find(';', i + 1);
    const auto space = value.find_first_of(" \t\r\n<", i + 1);
    std::size_t end = semi;
    bool has_semicolon = semi != std::string::npos && (space == std::string::npos || semi < space);
    if (!has_semicolon) end = space;
    if (end == std::string::npos || end <= i + 1 || end - i > 16) { out.push_back(value[i++]); continue; }
    const auto entity = lower_ascii(value.substr(i + 1, end - i - 1));
    std::string decoded;
    if (entity == "amp") decoded = "&";
    else if (entity == "lt") decoded = "<";
    else if (entity == "gt") decoded = ">";
    else if (entity == "quot") decoded = "\"";
    else if (entity == "apos" || entity == "#39") decoded = "'";
    else if (entity == "nbsp") decoded = "\xC2\xA0";
    else if (!entity.empty() && entity[0] == '#') {
      try {
        const bool hex = entity.size() > 2 && entity[1] == 'x';
        const auto number = std::stoul(entity.substr(hex ? 2 : 1), nullptr, hex ? 16 : 10);
        if (number <= 0x7f) decoded.push_back(static_cast<char>(number));
        else if (number <= 0x7ff) { decoded.push_back(static_cast<char>(0xc0 | (number >> 6))); decoded.push_back(static_cast<char>(0x80 | (number & 0x3f))); }
        else if (number <= 0xffff) { decoded.push_back(static_cast<char>(0xe0 | (number >> 12))); decoded.push_back(static_cast<char>(0x80 | ((number >> 6) & 0x3f))); decoded.push_back(static_cast<char>(0x80 | (number & 0x3f))); }
      } catch (...) {}
    }
    if (decoded.empty()) { out.push_back(value[i++]); continue; }
    out += decoded;
    i = end + (has_semicolon ? 1 : 0);
  }
  return out;
}

std::string normalize_text(std::string value, bool preserve_whitespace) {
  value = decode_basic_entities(std::move(value));
  if (preserve_whitespace) {
    return value;
  }

  std::string out;
  out.reserve(value.size());
  bool previous_space = false;
  bool has_non_space = false;
  for (char c : value) {
    const bool is_space = std::isspace(static_cast<unsigned char>(c)) != 0;
    if (is_space) {
      if (!previous_space) out.push_back(' ');
      previous_space = true;
    } else {
      out.push_back(c);
      previous_space = false;
      has_non_space = true;
    }
  }

  // Formatting-only indentation between tags has no rendered text. For text that
  // contains content, preserve a single leading/trailing space because those
  // boundaries are significant across inline elements ("User Agent <span>...").
  return has_non_space ? out : std::string{};
}

bool is_preformatted_element(const std::string& name) {
  return name == "pre" || name == "textarea" || name == "listing" || name == "xmp";
}

std::size_t find_case_insensitive(std::string_view haystack, std::string_view needle, std::size_t start) {
  if (needle.empty() || haystack.size() < needle.size()) {
    return std::string::npos;
  }
  for (std::size_t i = start; i + needle.size() <= haystack.size(); ++i) {
    bool match = true;
    for (std::size_t j = 0; j < needle.size(); ++j) {
      const auto a = static_cast<unsigned char>(haystack[i + j]);
      const auto b = static_cast<unsigned char>(needle[j]);
      if (std::tolower(a) != std::tolower(b)) {
        match = false;
        break;
      }
    }
    if (match) {
      return i;
    }
  }
  return std::string::npos;
}

std::string extract_body(std::string_view html) {
  const auto body_start = find_case_insensitive(html, "<body", 0);
  if (body_start == std::string::npos) {
    return std::string(html);
  }
  const auto body_tag_end = html.find('>', body_start);
  if (body_tag_end == std::string::npos) {
    return std::string(html);
  }
  const auto body_end = find_case_insensitive(html, "</body", body_tag_end + 1);
  if (body_end == std::string::npos) {
    return std::string(html.substr(body_tag_end + 1));
  }
  return std::string(html.substr(body_tag_end + 1, body_end - body_tag_end - 1));
}

std::vector<Token> tokenize_html(std::string_view source) {
  std::vector<Token> tokens;
  std::size_t i = 0;
  std::size_t preformatted_depth = 0;
  while (i < source.size()) {
    if (source[i] != '<') {
      const auto start = i;
      while (i < source.size() && source[i] != '<') {
        ++i;
      }
      auto text = normalize_text(std::string(source.substr(start, i - start)), preformatted_depth != 0);
      if (!text.empty()) {
        Token token;
        token.kind = Token::Kind::Text;
        token.text = std::move(text);
        tokens.push_back(std::move(token));
      }
      continue;
    }

    if (source.substr(i, 4) == "<!--") {
      const auto end = source.find("-->", i + 4);
      i = end == std::string::npos ? source.size() : end + 3;
      continue;
    }
    if (i + 2 < source.size() && source[i + 1] == '!') {
      const auto end = source.find('>', i + 2);
      i = end == std::string::npos ? source.size() : end + 1;
      continue;
    }
    if (i + 2 < source.size() && source[i + 1] == '?') {
      const auto end = source.find("?>", i + 2);
      i = end == std::string::npos ? source.size() : end + 2;
      continue;
    }

    ++i; // '<'
    bool closing = false;
    if (i < source.size() && source[i] == '/') {
      closing = true;
      ++i;
    }
    skip_space(source, i);
    auto name = read_name(source, i);
    if (name.empty()) {
      const auto end = source.find('>', i);
      i = end == std::string::npos ? source.size() : end + 1;
      continue;
    }

    if (closing) {
      const auto end = source.find('>', i);
      i = end == std::string::npos ? source.size() : end + 1;
      if (is_preformatted_element(name) && preformatted_depth != 0) {
        --preformatted_depth;
      }
      Token token;
      token.kind = Token::Kind::EndTag;
      token.name = std::move(name);
      tokens.push_back(std::move(token));
      continue;
    }

    Token token;
    token.kind = Token::Kind::StartTag;
    token.name = std::move(name);

    while (i < source.size()) {
      skip_space(source, i);
      if (i >= source.size()) {
        break;
      }
      if (source[i] == '>') {
        ++i;
        break;
      }
      if (source[i] == '/' && i + 1 < source.size() && source[i + 1] == '>') {
        token.self_closing = true;
        i += 2;
        break;
      }

      auto attr_name = read_name(source, i);
      if (attr_name.empty()) {
        ++i;
        continue;
      }
      skip_space(source, i);
      std::string attr_value;
      if (i < source.size() && source[i] == '=') {
        ++i;
        skip_space(source, i);
        if (i < source.size() && (source[i] == '"' || source[i] == '\'')) {
          const char quote = source[i++];
          const auto start = i;
          while (i < source.size() && source[i] != quote) {
            ++i;
          }
          attr_value = std::string(source.substr(start, i - start));
          if (i < source.size()) {
            ++i;
          }
        } else {
          const auto start = i;
          while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) == 0 && source[i] != '>' && source[i] != '/') {
            ++i;
          }
          attr_value = std::string(source.substr(start, i - start));
        }
      }
      token.attributes.push_back({std::move(attr_name), decode_basic_entities(std::move(attr_value))});
    }

    const auto token_name = token.name;
    if (token_name == "script") token.executable_script = script_is_executable(token.attributes);
    const bool executable_script = token.executable_script;
    tokens.push_back(std::move(token));
    if (is_preformatted_element(token_name)) ++preformatted_depth;

    if (token_name == "script" || token_name == "style") {
      const std::string end_tag = "</" + token_name;
      const auto end = find_case_insensitive(source, end_tag, i);
      if (end == std::string::npos) {
        if (token_name == "script" && !executable_script && i < source.size()) {
          Token text_token; text_token.kind = Token::Kind::Text; text_token.text = std::string(source.substr(i));
          tokens.push_back(std::move(text_token));
        }
        i = source.size();
      } else {
        if (token_name == "script" && !executable_script && end > i) {
          Token text_token; text_token.kind = Token::Kind::Text; text_token.text = std::string(source.substr(i, end - i));
          tokens.push_back(std::move(text_token));
        }
        const auto close = source.find('>', end);
        i = close == std::string::npos ? source.size() : close + 1;
        Token end_token;
        end_token.kind = Token::Kind::EndTag;
        end_token.name = token_name;
        end_token.executable_script = executable_script;
        tokens.push_back(std::move(end_token));
      }
    }
  }
  return tokens;
}

class StringPool {
public:
  std::uint32_t intern(const std::string& value) {
    const auto found = map_.find(value);
    if (found != map_.end()) {
      return found->second;
    }
    const auto id = static_cast<std::uint32_t>(values_.size());
    values_.push_back(value);
    map_.emplace(value, id);
    return id;
  }

  const std::vector<std::string>& values() const { return values_; }

private:
  std::vector<std::string> values_;
  std::unordered_map<std::string, std::uint32_t> map_;
};

std::uint32_t string_stream_next(std::uint32_t& state) {
  state ^= state << 13u;
  state ^= state >> 17u;
  state ^= state << 5u;
  return state;
}

std::vector<unsigned char> encode_string_table(const std::vector<std::string>& strings, std::uint32_t build_seed) {
  std::vector<unsigned char> out;
  out.insert(out.end(), {'V', 'S', 'T', 'R', '0', '0', '1', '1'});
  append_u32(out, 2);
  append_u32(out, static_cast<std::uint32_t>(strings.size()));
  const std::uint32_t table_seed = build_seed ^ 0xA53C9E17u ^ static_cast<std::uint32_t>(strings.size() * 0x9E3779B9u);
  append_u32(out, table_seed);
  std::uint32_t cursor = 0;
  for (std::uint32_t i = 0; i < strings.size(); ++i) {
    const auto& value = strings[i];
    append_u32(out, cursor);
    append_u32(out, static_cast<std::uint32_t>(value.size()));
    append_u32(out, 0xC3D2E1F0u ^ (i * 0x45D9F3Bu) ^ static_cast<std::uint32_t>(value.size()));
    cursor += static_cast<std::uint32_t>(value.size());
  }
  for (std::uint32_t i = 0; i < strings.size(); ++i) {
    const auto& value = strings[i];
    std::uint32_t state = table_seed ^ (0xC3D2E1F0u ^ (i * 0x45D9F3Bu) ^ static_cast<std::uint32_t>(value.size())) ^ (static_cast<std::uint32_t>(value.size()) * 0x27D4EB2Du);
    for (const char raw_ch : value) {
    const auto ch = static_cast<unsigned char>(raw_ch);
      const auto word = string_stream_next(state);
      out.push_back(static_cast<unsigned char>(ch ^ static_cast<unsigned char>(word & 0xffu)));
    }
  }
  return out;
}

std::vector<unsigned char> encode_route_table(const std::vector<CompiledHtmlRoute>& routes, const std::vector<std::string>& strings) {
  std::unordered_map<std::string, std::uint32_t> string_ids;
  for (std::uint32_t i = 0; i < strings.size(); ++i) {
    string_ids.emplace(strings[i], i);
  }

  std::vector<unsigned char> out;
  out.insert(out.end(), {'V', 'R', 'T', 'E', '0', '0', '0', '3'});
  append_u32(out, 1);
  append_u32(out, static_cast<std::uint32_t>(routes.size()));
  for (const auto& route : routes) {
    append_u32(out, string_ids.at(route.route));
    append_u32(out, string_ids.at(route.source_path));
    append_u32(out, route.bytecode_offset);
    append_u32(out, route.bytecode_size);
    append_u32(out, route.instruction_count);
    append_u32(out, 0);
  }
  return out;
}

std::vector<unsigned char> make_bytecode_section(const std::vector<unsigned char>& encoded, std::uint32_t instruction_count) {
  std::vector<unsigned char> out;
  out.insert(out.end(), {'V', 'B', 'C', 'O', 'D', 'E', '0', '3'});
  append_u32(out, 1);
  append_u32(out, 16);
  append_u32(out, instruction_count);
  append_u32(out, 0);
  out.insert(out.end(), encoded.begin(), encoded.end());
  return out;
}

std::string make_preview(const CompiledHtmlRoutes& compiled) {
  std::ostringstream out;
  out << "compiled_routes=" << compiled.routes.size() << "\n";
  out << "strings=" << compiled.strings.size() << "\n";
  for (const auto& route : compiled.routes) {
    out << "\nroute " << route.route << " source=" << route.source_path << "\n";
    out << "  bytecode_offset=" << route.bytecode_offset
        << " bytecode_size=" << route.bytecode_size
        << " instructions=" << route.instruction_count << "\n";
    for (const auto& ins : route.program) {
      out << "  " << venom::vm::opcode_name(ins.opcode)
          << " " << ins.a << " " << ins.b << " " << ins.c << "\n";
    }
  }
  return out.str();
}


void remap_instruction_strings(venom::vm::Instruction& ins, const std::vector<std::uint32_t>& old_to_new) {
  const auto remap = [&](std::uint32_t value) -> std::uint32_t {
    if (value >= old_to_new.size()) {
      raise_error("VENOM-E5000", "string id remap out of range");
    }
    return old_to_new[value];
  };

  switch (ins.opcode) {
    case venom::vm::LogicalOpcode::CreateElement:
    case venom::vm::LogicalOpcode::SetText:
      ins.a = remap(ins.a);
      break;
    case venom::vm::LogicalOpcode::SetAttribute:
      ins.a = remap(ins.a);
      ins.b = remap(ins.b);
      break;
    default:
      break;
  }
}

void apply_string_shuffle(CompiledHtmlRoutes& compiled, const venom::vm::PolymorphicPlan& plan) {
  if (!plan.shuffle_strings || compiled.strings.size() < 2) {
    return;
  }

  std::vector<std::uint32_t> order(compiled.strings.size());
  std::iota(order.begin(), order.end(), 0u);
  venom::vm::DiversificationRng rng(plan, "string-table-order");
  std::shuffle(order.begin(), order.end(), rng);

  std::vector<std::uint32_t> old_to_new(order.size());
  std::vector<std::string> shuffled(order.size());
  for (std::uint32_t new_id = 0; new_id < order.size(); ++new_id) {
    const auto old_id = order[new_id];
    old_to_new[old_id] = new_id;
    shuffled[new_id] = compiled.strings[old_id];
  }

  compiled.strings = std::move(shuffled);
  for (auto& route : compiled.routes) {
    for (auto& ins : route.program) {
      remap_instruction_strings(ins, old_to_new);
    }
  }
}

void apply_route_shuffle(CompiledHtmlRoutes& compiled, const venom::vm::PolymorphicPlan& plan) {
  if (!plan.shuffle_routes || compiled.routes.size() < 2) {
    return;
  }
  venom::vm::DiversificationRng rng(plan, "route-order");
  std::shuffle(compiled.routes.begin(), compiled.routes.end(), rng);
}

void encode_compiled_routes(CompiledHtmlRoutes& compiled, const venom::vm::PolymorphicPlan& plan) {
  std::vector<unsigned char> encoded_stream;
  std::uint32_t total_instructions = 0;
  for (auto& route : compiled.routes) {
    const auto encoded = venom::vm::encode_program(route.program, plan);
    route.bytecode_offset = static_cast<std::uint32_t>(encoded_stream.size());
    route.bytecode_size = static_cast<std::uint32_t>(encoded.size());
    route.instruction_count = static_cast<std::uint32_t>(route.program.size());
    encoded_stream.insert(encoded_stream.end(), encoded.begin(), encoded.end());
    total_instructions += route.instruction_count;
  }
  compiled.string_table = encode_string_table(compiled.strings, plan.seed);
  compiled.route_table = encode_route_table(compiled.routes, compiled.strings);
  compiled.bytecode = make_bytecode_section(encoded_stream, total_instructions);
}

void compile_tokens_to_program(const std::vector<Token>& tokens, StringPool& strings, venom::vm::Program& program) {
  std::vector<std::string> open_elements;

  const auto close_current = [&]() {
    program.push_back({venom::vm::LogicalOpcode::LeaveElement, 0, 0, 0});
    program.push_back({venom::vm::LogicalOpcode::AppendChild, 0, 0, 0});
  };

  const auto close_through = [&](std::string_view name) {
    const auto match = std::find(open_elements.rbegin(), open_elements.rend(), name);
    if (match == open_elements.rend()) return false;
    const auto close_count = static_cast<std::size_t>(std::distance(open_elements.rbegin(), match)) + 1u;
    for (std::size_t i = 0; i < close_count; ++i) {
      close_current();
      open_elements.pop_back();
    }
    return true;
  };

  const auto apply_implied_start_closures = [&](std::string_view name) {
    if (name == "li") close_through("li");
    if (name == "dt" || name == "dd") {
      if (!close_through("dt")) close_through("dd");
    }
    if (name == "option" || name == "optgroup") close_through("option");
    if (name == "tr") close_through("tr");
    if (name == "td" || name == "th") {
      if (!close_through("td")) close_through("th");
    }
    if (name == "thead" || name == "tbody" || name == "tfoot") {
      if (!close_through("thead") && !close_through("tbody")) close_through("tfoot");
    }
    static constexpr std::string_view kClosesParagraph[] = {
      "address", "article", "aside", "blockquote", "div", "dl", "fieldset", "footer",
      "form", "h1", "h2", "h3", "h4", "h5", "h6", "header", "hr", "main", "nav",
      "ol", "p", "pre", "section", "table", "ul"
    };
    if (std::find(std::begin(kClosesParagraph), std::end(kClosesParagraph), name) != std::end(kClosesParagraph)) {
      close_through("p");
    }
  };

  for (const auto& token : tokens) {
    if (token.kind == Token::Kind::Text) {
      if (!token.text.empty()) {
        const auto text_id = strings.intern(token.text);
        program.push_back({venom::vm::LogicalOpcode::SetText, text_id, 0, 0});
      }
      continue;
    }

    if (token.kind == Token::Kind::StartTag) {
      if ((token.name == "script" && token.executable_script) || token.name == "style") continue;
      apply_implied_start_closures(token.name);
      const auto tag_id = strings.intern(token.name);
      program.push_back({venom::vm::LogicalOpcode::CreateElement, tag_id, 0, 0});
      program.push_back({venom::vm::LogicalOpcode::EnterElement, 0, 0, 0});
      for (const auto& attr : token.attributes) {
        if (attr.name.empty()) {
          continue;
        }
        const auto name_id = strings.intern(attr.name);
        const auto value_id = strings.intern(attr.value);
        program.push_back({venom::vm::LogicalOpcode::SetAttribute, name_id, value_id, 0});
      }
      if (token.self_closing || is_void_element(token.name)) {
        close_current();
      } else {
        open_elements.push_back(token.name);
      }
      continue;
    }

    if (token.kind == Token::Kind::EndTag) {
      if ((token.name == "script" && token.executable_script) || token.name == "style" || is_void_element(token.name)) continue;

      // Browsers ignore unmatched closing tags. Do the same rather than
      // emitting a VM pop that can underflow the synthetic route root.
      close_through(token.name);
    }
  }

  // HTML permits omitted closing tags. Close any remaining compiler-owned
  // elements so the VM stream is always structurally balanced.
  while (!open_elements.empty()) {
    close_current();
    open_elements.pop_back();
  }

  program.push_back({venom::vm::LogicalOpcode::Halt, 0, 0, 0});
}

struct DocumentShell {
  std::vector<Attribute> html_attributes;
  std::vector<Attribute> body_attributes;
  std::string title = "Venom Runtime";
};

std::string trim_ascii(std::string value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) value.erase(value.begin());
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) value.pop_back();
  return value;
}

std::string escape_html_attribute(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const char c : value) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

std::string escape_html_text(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const char c : value) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

bool is_safe_shell_attribute(const Attribute& attr) {
  if (attr.name.empty()) return false;
  const auto name = lower_ascii(attr.name);
  if (name.size() >= 2 && name[0] == 'o' && name[1] == 'n') return false;
  return name != "nonce" && name != "integrity" && name != "crossorigin";
}

std::string render_shell_attributes(const std::vector<Attribute>& attributes, bool ensure_lang) {
  std::ostringstream out;
  bool has_lang = false;
  for (const auto& attr : attributes) {
    if (!is_safe_shell_attribute(attr)) continue;
    if (lower_ascii(attr.name) == "lang") has_lang = true;
    out << ' ' << attr.name;
    if (!attr.value.empty()) out << "=\"" << escape_html_attribute(attr.value) << "\"";
  }
  if (ensure_lang && !has_lang) out << " lang=\"en\"";
  return out.str();
}

const SiteFile* document_shell_file(const SiteGraph& graph) {
  const SiteFile* first = nullptr;
  for (const auto& file : graph.files) {
    if (file.extension != ".html" && file.extension != ".htm") continue;
    if (file.relative == "index.html") return &file;
    if (first == nullptr) first = &file;
  }
  return first;
}

DocumentShell parse_document_shell(const SiteGraph& graph) {
  DocumentShell shell;
  const auto* file = document_shell_file(graph);
  if (file == nullptr) return shell;

  const auto tokens = tokenize_html(to_string(file->bytes));
  bool in_title = false;
  std::string title;
  for (const auto& token : tokens) {
    if (token.kind == Token::Kind::StartTag) {
      if (token.name == "html" && shell.html_attributes.empty()) shell.html_attributes = token.attributes;
      if (token.name == "body" && shell.body_attributes.empty()) shell.body_attributes = token.attributes;
      if (token.name == "title") in_title = true;
      continue;
    }
    if (token.kind == Token::Kind::EndTag) {
      if (token.name == "title") in_title = false;
      continue;
    }
    if (in_title && token.kind == Token::Kind::Text) title += token.text;
  }
  title = trim_ascii(std::move(title));
  if (!title.empty()) shell.title = std::move(title);
  return shell;
}

} // namespace

std::string make_bootstrap_html(const SiteGraph& graph,
                                const std::string& loader_public_path,
                                const std::string& style_public_path,
                                const std::string& loader_integrity,
                                const std::string& style_integrity,
                                const std::vector<HtmlPreloadHint>& preload_hints) {
  const auto shell = parse_document_shell(graph);
  const auto style_sri = style_integrity.empty() ? std::string{} : " integrity=\"" + style_integrity + "\" crossorigin=\"anonymous\"";
  const auto loader_sri = loader_integrity.empty() ? std::string{} : " integrity=\"" + loader_integrity + "\" crossorigin=\"anonymous\"";
  std::ostringstream out;
  out << "<!doctype html>\n"
      << "<html" << render_shell_attributes(shell.html_attributes, true) << ">\n"
      << "<head>\n"
      << "  <meta charset=\"utf-8\">\n"
      << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
      << "  <title>" << escape_html_text(shell.title) << "</title>\n"
      << "  <link rel=\"icon\" href=\"data:,\">\n"
      << "  <link rel=\"stylesheet\" href=\"" << style_public_path << "\"" << style_sri << ">\n";
  for (const auto& hint : preload_hints) {
    if (hint.href.empty() || hint.relation.empty()) continue;
    out << "  <link rel=\"" << escape_html_attribute(hint.relation) << "\" href=\""
        << escape_html_attribute(hint.href) << "\"";
    if (!hint.as_type.empty()) out << " as=\"" << escape_html_attribute(hint.as_type) << "\"";
    if (!hint.mime_type.empty()) out << " type=\"" << escape_html_attribute(hint.mime_type) << "\"";
    if (hint.crossorigin) out << " crossorigin=\"anonymous\"";
    out << ">\n";
  }
  out << "</head>\n"
      << "<body" << render_shell_attributes(shell.body_attributes, false) << ">\n"
      << "  <noscript>This protected runtime requires JavaScript.</noscript>\n"
      << "  <div id=\"venom-root\"></div>\n"
      << "  <script type=\"module\" src=\"" << loader_public_path << "\"" << loader_sri << "></script>\n"
      << "</body>\n"
      << "</html>\n";
  return out.str();
}


std::string html_manifest(const SiteGraph& graph, const CompiledHtmlRoutes* compiled, const std::string& asset_manifest) {
  std::ostringstream out;
  out << "venom_manifest_version: 7\n";
  out << "files: " << graph.files.size() << "\n";
  out << "routes: " << graph.routes.size() << "\n";
  out << "assets: " << graph.assets.size() << "\n";
  if (compiled != nullptr) {
    out << "compiled_routes: " << compiled->routes.size() << "\n";
    out << "vm_strings: " << compiled->strings.size() << "\n";
    out << "vm_bytecode_bytes: " << compiled->bytecode.size() << "\n";
  }
  if (!asset_manifest.empty()) {
    out << "asset_manifest: " << asset_manifest << "\n";
  }
  out << "route_list:\n";
  for (const auto& route : graph.routes) {
    out << "  - " << route << "\n";
  }
  return out.str();
}


CompiledHtmlRoutes compile_html_routes(const SiteGraph& graph, const venom::vm::PolymorphicPlan& plan) {
  StringPool strings;
  CompiledHtmlRoutes compiled;

  std::vector<const SiteFile*> html_files;
  for (const auto& file : graph.files) {
    if (file.extension == ".html" || file.extension == ".htm") {
      html_files.push_back(&file);
    }
  }
  std::sort(html_files.begin(), html_files.end(), [](const SiteFile* a, const SiteFile* b) {
    return a->relative < b->relative;
  });

  for (const auto* file : html_files) {
    CompiledHtmlRoute route;
    route.route = route_from_html_path(file->relative);
    route.source_path = file->relative;
    strings.intern(route.route);
    strings.intern(route.source_path);

    const auto html = extract_body(to_string(file->bytes));
    const auto tokens = tokenize_html(html);
    compile_tokens_to_program(tokens, strings, route.program);
    route.instruction_count = static_cast<std::uint32_t>(route.program.size());
    compiled.routes.push_back(std::move(route));
  }

  compiled.strings = strings.values();
  apply_string_shuffle(compiled, plan);
  apply_route_shuffle(compiled, plan);
  encode_compiled_routes(compiled, plan);
  compiled.preview = make_preview(compiled);
  return compiled;
}

} // namespace venom::compiler
