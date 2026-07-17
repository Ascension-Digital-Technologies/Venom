#include "compiler/pipeline/jsx_frontend.hpp"
#include "compiler/core/diagnostic.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace venom::compiler::jsx {
namespace {

bool ident_start(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '$';
}

bool ident_char(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '$' || c == '-' || c == ':';
}

std::string quote(const std::string& value) {
  std::string out = "\"";
  for (const char raw_c : value) { const auto c = static_cast<unsigned char>(raw_c);
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          static constexpr char hex[] = "0123456789abcdef";
          out += "\\u00";
          out += hex[(c >> 4) & 0xf];
          out += hex[c & 0xf];
        } else out.push_back(static_cast<char>(c));
    }
  }
  out += '"';
  return out;
}

[[noreturn]] void fail(const std::string& source, const std::string& filename,
                       std::size_t offset, const std::string& detail) {
  std::size_t line = 1, column = 0, line_begin = 0;
  for (std::size_t i = 0; i < std::min(offset, source.size()); ++i) {
    if (source[i] == '\n') { ++line; column = 0; line_begin = i + 1; }
    else ++column;
  }
  const auto line_end = source.find('\n', line_begin);
  SourceLocation location;
  location.file = filename;
  location.line = line;
  location.column = column;
  location.source_line = source.substr(line_begin, line_end == std::string::npos
    ? std::string::npos : line_end - line_begin);
  raise_diagnostic("VENOM-E2503", "JSX lowering failed", std::move(location), detail,
                   "Use balanced JSX tags and JavaScript expressions, then rebuild.",
                   "docs/reference/diagnostics.md#venom-e2503");
}

std::string normalize_text(std::string_view text) {
  std::string result;
  bool pending_space = false;
  for (const char c : text) {
    if (std::isspace(static_cast<unsigned char>(c)) != 0) {
      pending_space = !result.empty();
    } else {
      if (pending_space) result.push_back(' ');
      result.push_back(c);
      pending_space = false;
    }
  }
  return result;
}

class Parser {
public:
  Parser(const std::string& source, const std::string& filename, std::size_t start)
      : source_(source), filename_(filename), position_(start) {}

  std::string element() {
    const auto element_start = position_;
    expect('<');
    if (peek() == '>') {
      ++position_;
      auto children = parse_children("", true);
      return emit("React.Fragment", {}, children);
    }

    const auto tag = parse_name();
    if (tag.empty()) fail(source_, filename_, element_start, "Expected a JSX tag name.");

    std::vector<std::string> properties;
    skip_space();
    bool self_closing = false;
    while (position_ < source_.size()) {
      if (starts_with("/>")) { position_ += 2; self_closing = true; break; }
      if (peek() == '>') { ++position_; break; }

      if (peek() == '{') {
        const auto expression = braced_expression();
        auto trimmed = trim(expression);
        if (trimmed.rfind("...", 0) != 0)
          fail(source_, filename_, position_, "Only JSX spread attributes may appear directly inside a start tag.");
        properties.push_back("...(" + trim(trimmed.substr(3)) + ")");
        skip_space();
        continue;
      }

      const auto name = parse_name();
      if (name.empty()) fail(source_, filename_, position_, "Expected a JSX attribute name.");
      skip_space();
      std::string value = "true";
      if (peek() == '=') {
        ++position_;
        skip_space();
        if (peek() == '"' || peek() == '\'') value = quoted_attribute();
        else if (peek() == '{') {
          value = trim(braced_expression());
          if (value.empty()) value = "undefined";
          else value = "(" + value + ")";
        } else fail(source_, filename_, position_, "Expected a quoted or braced JSX attribute value.");
      }
      properties.push_back(quote(name) + ":" + value);
      skip_space();
    }

    std::vector<std::string> children;
    if (!self_closing) children = parse_children(tag, false);
    return emit(tag_expression(tag), properties, children);
  }

  std::size_t position() const { return position_; }

private:
  const std::string& source_;
  const std::string& filename_;
  std::size_t position_ = 0;

  char peek() const { return position_ < source_.size() ? source_[position_] : '\0'; }
  bool starts_with(std::string_view value) const {
    return position_ + value.size() <= source_.size() &&
           std::string_view(source_).substr(position_, value.size()) == value;
  }
  void expect(char value) {
    if (peek() != value) fail(source_, filename_, position_, std::string("Expected '") + value + "'.");
    ++position_;
  }
  void skip_space() {
    while (position_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[position_])) != 0) ++position_;
  }
  static std::string trim(std::string value) {
    auto first = value.begin();
    while (first != value.end() && std::isspace(static_cast<unsigned char>(*first)) != 0) ++first;
    auto last = value.end();
    while (last != first && std::isspace(static_cast<unsigned char>(*(last - 1))) != 0) --last;
    return std::string(first, last);
  }
  std::string parse_name() {
    const auto begin = position_;
    while (position_ < source_.size() && (ident_char(source_[position_]) || source_[position_] == '.')) ++position_;
    return source_.substr(begin, position_ - begin);
  }
  std::string quoted_attribute() {
    const char delimiter = peek();
    ++position_;
    std::string value;
    bool escape = false;
    while (position_ < source_.size()) {
      const char c = source_[position_++];
      if (escape) { value.push_back(c); escape = false; continue; }
      if (c == '\\') { value.push_back(c); escape = true; continue; }
      if (c == delimiter) return quote(value);
      value.push_back(c);
    }
    fail(source_, filename_, position_, "Unterminated JSX attribute string.");
  }
  std::string braced_expression() {
    const auto open = position_;
    expect('{');
    const auto begin = position_;
    int depth = 1;
    enum class State { Code, Single, Double, Template, LineComment, BlockComment } state = State::Code;
    bool escape = false;
    while (position_ < source_.size()) {
      const char c = source_[position_];
      const char next = position_ + 1 < source_.size() ? source_[position_ + 1] : '\0';
      if (state == State::Code) {
        if (c == '/' && next == '/') { state = State::LineComment; position_ += 2; continue; }
        if (c == '/' && next == '*') { state = State::BlockComment; position_ += 2; continue; }
        if (c == '\'') { state = State::Single; escape = false; ++position_; continue; }
        if (c == '"') { state = State::Double; escape = false; ++position_; continue; }
        if (c == '`') { state = State::Template; escape = false; ++position_; continue; }
        if (c == '{') ++depth;
        else if (c == '}' && --depth == 0) {
          const auto value = source_.substr(begin, position_ - begin);
          ++position_;
          return value;
        }
        ++position_;
        continue;
      }
      if (state == State::LineComment) { if (c == '\n') state = State::Code; ++position_; continue; }
      if (state == State::BlockComment) {
        if (c == '*' && next == '/') { state = State::Code; position_ += 2; }
        else ++position_;
        continue;
      }
      if (escape) { escape = false; ++position_; continue; }
      if (c == '\\') { escape = true; ++position_; continue; }
      if ((state == State::Single && c == '\'') || (state == State::Double && c == '"') ||
          (state == State::Template && c == '`')) state = State::Code;
      ++position_;
    }
    fail(source_, filename_, open, "Unterminated JSX expression container.");
  }
  std::vector<std::string> parse_children(const std::string& tag, bool fragment) {
    std::vector<std::string> children;
    while (position_ < source_.size()) {
      if (fragment && starts_with("</>")) { position_ += 3; return children; }
      if (!fragment && starts_with("</")) {
        position_ += 2;
        const auto close_tag = parse_name();
        skip_space();
        expect('>');
        if (close_tag != tag)
          fail(source_, filename_, position_, "JSX closing tag </" + close_tag + "> does not match <" + tag + ">.");
        return children;
      }
      if (peek() == '<') {
        children.push_back(element());
        continue;
      }
      if (peek() == '{') {
        auto expression = trim(braced_expression());
        if (expression.empty() || expression.rfind("/*", 0) == 0) continue;
        children.push_back("(" + expression + ")");
        continue;
      }
      const auto begin = position_;
      while (position_ < source_.size() && peek() != '<' && peek() != '{') ++position_;
      auto text = normalize_text(std::string_view(source_).substr(begin, position_ - begin));
      if (!text.empty()) children.push_back(quote(text));
    }
    fail(source_, filename_, position_, fragment ? "Unterminated JSX fragment." : "Unterminated JSX element <" + tag + ">.");
  }
  static std::string tag_expression(const std::string& tag) {
    if (!tag.empty() && (std::islower(static_cast<unsigned char>(tag.front())) != 0 || tag.find('-') != std::string::npos))
      return quote(tag);
    return tag;
  }
  static std::string emit(const std::string& tag, const std::vector<std::string>& properties,
                          const std::vector<std::string>& children) {
    std::string output = "React.createElement(" + tag + ",";
    if (properties.empty()) output += "null";
    else {
      output += "{";
      for (std::size_t i = 0; i < properties.size(); ++i) {
        if (i) output += ',';
        output += properties[i];
      }
      output += "}";
    }
    for (const auto& child : children) output += "," + child;
    output += ")";
    return output;
  }
};

std::string code_mask(const std::string& source) {
  std::string mask = source;
  enum class State { Code, Single, Double, Template, LineComment, BlockComment } state = State::Code;
  bool escape = false;
  for (std::size_t i = 0; i < source.size(); ++i) {
    const char c = source[i];
    const char n = i + 1 < source.size() ? source[i + 1] : '\0';
    if (state == State::Code) {
      if (c == '/' && n == '/') { mask[i] = mask[i + 1] = ' '; ++i; state = State::LineComment; }
      else if (c == '/' && n == '*') { mask[i] = mask[i + 1] = ' '; ++i; state = State::BlockComment; }
      else if (c == '\'') { mask[i] = ' '; state = State::Single; escape = false; }
      else if (c == '"') { mask[i] = ' '; state = State::Double; escape = false; }
      else if (c == '`') { mask[i] = ' '; state = State::Template; escape = false; }
      continue;
    }
    if (c != '\n' && c != '\r') mask[i] = ' ';
    if (state == State::LineComment) { if (c == '\n') state = State::Code; continue; }
    if (state == State::BlockComment) { if (c == '*' && n == '/') { mask[i + 1] = ' '; ++i; state = State::Code; } continue; }
    if (escape) { escape = false; continue; }
    if (c == '\\') { escape = true; continue; }
    if ((state == State::Single && c == '\'') || (state == State::Double && c == '"') || (state == State::Template && c == '`')) state = State::Code;
  }
  return mask;
}

bool likely_jsx(std::string_view mask, std::size_t i) {
  if (i >= mask.size() || mask[i] != '<' || i + 1 >= mask.size()) return false;
  const char next = mask[i + 1];
  if (!(ident_start(next) || next == '>')) return false;
  std::size_t p = i;
  while (p > 0 && std::isspace(static_cast<unsigned char>(mask[p - 1])) != 0) --p;
  if (p == 0) return true;
  const char previous = mask[p - 1];
  return previous == '=' || previous == '(' || previous == '[' || previous == '{' || previous == ',' ||
         previous == ':' || previous == ';' || previous == '>' || previous == '\n' ||
         std::string_view("return").find(previous) != std::string_view::npos;
}

} // namespace

LowerResult lower_once(const std::string& source, const std::string& filename) {
  LowerResult result;
  const auto mask = code_mask(source);
  std::size_t cursor = 0;
  while (cursor < source.size()) {
    std::size_t candidate = mask.find('<', cursor);
    if (candidate == std::string::npos) {
      result.javascript.append(source, cursor, std::string::npos);
      break;
    }
    if (!likely_jsx(mask, candidate)) {
      result.javascript.append(source, cursor, candidate - cursor + 1);
      cursor = candidate + 1;
      continue;
    }
    // In TSX, generic arrow functions use `<T,>(...)`; do not confuse them with JSX.
    const auto generic_close = mask.find('>', candidate + 1);
    if (generic_close != std::string::npos) {
      std::size_t after = generic_close + 1;
      while (after < mask.size() && std::isspace(static_cast<unsigned char>(mask[after])) != 0) ++after;
      if (after < mask.size() && mask[after] == '(') {
        result.javascript.append(source, cursor, candidate - cursor + 1);
        cursor = candidate + 1;
        continue;
      }
    }
    try {
      Parser parser(source, filename, candidate);
      const auto replacement = parser.element();
      result.javascript.append(source, cursor, candidate - cursor);
      result.javascript += replacement;
      cursor = parser.position();
      ++result.lowered_elements;
    } catch (const DiagnosticError&) {
      throw;
    }
  }
  return result;
}

LowerResult lower(const std::string& source, const std::string& filename) {
  LowerResult total;
  total.javascript = source;
  for (std::size_t pass = 0; pass < 32; ++pass) {
    auto current = lower_once(total.javascript, filename);
    total.lowered_elements += current.lowered_elements;
    total.javascript = std::move(current.javascript);
    if (current.lowered_elements == 0) return total;
  }
  fail(source, filename, 0, "JSX lowering exceeded the nested element limit.");
}

const char* frontend_name() { return "venom-native-jsx-classic"; }
const char* frontend_version() { return "1"; }

} // namespace venom::compiler::jsx
