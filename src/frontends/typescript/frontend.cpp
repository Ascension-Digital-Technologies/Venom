#include "venom/base/error.hpp"
#include "venom/frontends/typescript/frontend.hpp"
#include "venom/internal/frontends/typescript/typescript_runtime.hpp"
#include "venom/core/diagnostic.hpp"
#include "venom/internal/frontends/jsx/frontend.hpp"
#include "venom/package/hash.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <atomic>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <utility>

namespace venom::compiler::typescript {
namespace {

std::mutex g_cache_mutex;
bool g_cache_enabled = false;
bool g_cache_verbose = false;
std::filesystem::path g_cache_directory;
std::atomic<std::size_t> g_cache_hits{0};
std::atomic<std::size_t> g_cache_misses{0};
std::atomic<std::size_t> g_cache_writes{0};

bool ident_start(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '$'; }
bool ident_char(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '$'; }

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

bool boundary(std::string_view text, std::size_t pos, std::size_t len) {
  return (pos == 0 || !ident_char(text[pos - 1])) && (pos + len >= text.size() || !ident_char(text[pos + len]));
}

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
    if (state == State::BlockComment) { if (c == '*' && n == '/') { if (i + 1 < mask.size()) mask[i + 1] = ' '; ++i; state = State::Code; } continue; }
    if (escape) { escape = false; continue; }
    if (c == '\\') { escape = true; continue; }
    if ((state == State::Single && c == '\'') || (state == State::Double && c == '"') || (state == State::Template && c == '`')) state = State::Code;
  }
  return mask;
}

void blank(std::string& out, std::size_t begin, std::size_t end) {
  end = std::min(end, out.size());
  for (std::size_t i = begin; i < end; ++i) if (out[i] != '\n' && out[i] != '\r') out[i] = ' ';
}

std::size_t skip_space(std::string_view text, std::size_t i) {
  while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i])) != 0) ++i;
  return i;
}

std::size_t statement_end(std::string_view mask, std::size_t begin) {
  int paren = 0, bracket = 0, brace = 0, angle = 0;
  for (std::size_t i = begin; i < mask.size(); ++i) {
    switch (mask[i]) {
      case '(': ++paren; break; case ')': if (paren) --paren; break;
      case '[': ++bracket; break; case ']': if (bracket) --bracket; break;
      case '{': ++brace; break; case '}': if (brace) --brace; break;
      case '<': ++angle; break; case '>': if (angle) --angle; break;
      case ';': if (!paren && !bracket && !brace && !angle) return i + 1; break;
      default: break;
    }
  }
  return mask.size();
}

std::size_t matching(std::string_view mask, std::size_t open, char lhs, char rhs) {
  int depth = 0;
  for (std::size_t i = open; i < mask.size(); ++i) {
    if (mask[i] == lhs) ++depth;
    else if (mask[i] == rhs && --depth == 0) return i;
  }
  return std::string::npos;
}

[[noreturn]] void fail(const std::string& code, const std::string& summary, const std::string& source,
                       const std::string& filename, std::size_t offset, const std::string& detail,
                       const std::string& help) {
  std::size_t line = 1, col = 0, line_begin = 0;
  for (std::size_t i = 0; i < std::min(offset, source.size()); ++i) {
    if (source[i] == '\n') { ++line; col = 0; line_begin = i + 1; } else ++col;
  }
  const auto line_end = source.find('\n', line_begin);
  SourceLocation location;
  location.file = filename;
  location.line = line;
  location.column = col;
  location.source_line = source.substr(line_begin, line_end == std::string::npos ? std::string::npos : line_end - line_begin);
  raise_diagnostic(code, summary, std::move(location), detail, help,
                   "docs/reference/diagnostics.md#" + lower(code));
}


std::size_t consume_type(std::string_view mask, std::size_t begin, std::string_view stops) {
  int angle = 0, paren = 0, bracket = 0, brace = 0;
  for (std::size_t i = begin; i < mask.size(); ++i) {
    const char c = mask[i];
    if (!angle && !paren && !bracket && !brace && stops.find(c) != std::string_view::npos) return i;
    if (c == '<') ++angle; else if (c == '>' && angle) --angle;
    else if (c == '(') ++paren; else if (c == ')' && paren) --paren;
    else if (c == '[') ++bracket; else if (c == ']' && bracket) --bracket;
    else if (c == '{') ++brace; else if (c == '}' && brace) --brace;
  }
  return mask.size();
}

void erase_type_annotations(std::string& out, std::string& mask, std::size_t begin, std::size_t end, std::size_t& count) {
  int paren = 0, bracket = 0, brace = 0;
  for (std::size_t i = begin; i < end && i < mask.size(); ++i) {
    const char c = mask[i];
    if (c == '(') { ++paren; continue; }
    if (c == ')') { if (paren) --paren; continue; }
    if (c == '[') { ++bracket; continue; }
    if (c == ']') { if (bracket) --bracket; continue; }
    if (c == '{') { ++brace; continue; }
    if (c == '}') { if (brace) --brace; continue; }
    if (paren || bracket || brace) continue;
    if (c == '?' && i + 1 < end) {
      const auto n = skip_space(mask, i + 1);
      if (n < end && (mask[n] == ':' || mask[n] == ',' || mask[n] == ')' || mask[n] == '=')) { blank(out, i, i + 1); mask[i] = ' '; }
    }
    if (c != ':') continue;
    const auto type_begin = skip_space(mask, i + 1);
    const auto type_end = consume_type(mask, type_begin, ",)=;{");
    if (type_end <= type_begin) continue;
    blank(out, i, type_end); blank(mask, i, type_end); ++count;
    i = type_end ? type_end - 1 : type_end;
  }
}

std::string previous_identifier(std::string_view mask, std::size_t position) {
  while (position > 0 && std::isspace(static_cast<unsigned char>(mask[position - 1])) != 0) --position;
  const auto end = position;
  while (position > 0 && ident_char(mask[position - 1])) --position;
  return std::string(mask.substr(position, end - position));
}



bool word_at(std::string_view text, std::size_t pos, std::string_view word) {
  return pos + word.size() <= text.size() && text.substr(pos, word.size()) == word && boundary(text, pos, word.size());
}


void erase_generic_after_identifier(std::string& out, std::string& mask, std::size_t name_end,
                                    std::size_t& count) {
  const auto open = skip_space(mask, name_end);
  if (open >= mask.size() || mask[open] != '<') return;
  const auto close = matching(mask, open, '<', '>');
  if (close == std::string::npos) return;
  blank(out, open, close + 1);
  blank(mask, open, close + 1);
  ++count;
}

void erase_class_field_annotations(std::string& out, std::string& mask, std::size_t open,
                                   std::size_t close, std::size_t& count) {
  int brace = 0, paren = 0, bracket = 0;
  std::size_t statement = open + 1;
  for (std::size_t i = open + 1; i < close; ++i) {
    const char c = mask[i];
    if (c == '{') { ++brace; continue; }
    if (c == '}') { if (brace) --brace; continue; }
    if (brace) continue;
    if (c == '(') { ++paren; continue; }
    if (c == ')') { if (paren) --paren; continue; }
    if (c == '[') { ++bracket; continue; }
    if (c == ']') { if (bracket) --bracket; continue; }
    if (paren || bracket) continue;
    if (c == ';' || c == '\n') { statement = i + 1; continue; }
    if (c == '?' || c == '!') {
      const auto after = skip_space(mask, i + 1);
      if (after < close && mask[after] == ':') { blank(out, i, i + 1); mask[i] = ' '; }
    }
    if (c != ':') continue;
    // A class field annotation has no top-level `(` before the colon in this member.
    bool method_like = false;
    for (std::size_t j = statement; j < i; ++j) if (mask[j] == '(') { method_like = true; break; }
    if (method_like) continue;
    const auto type_begin = skip_space(mask, i + 1);
    const auto type_end = consume_type(mask, type_begin, "=;\n");
    if (type_end > type_begin) {
      const auto member_end = statement_end(mask, statement);
      const auto initializer = mask.find('=', type_end);
      if (initializer == std::string::npos || initializer >= member_end) {
        // Type-only class fields have no runtime value in Venom's erasure mode.
        blank(out, statement, member_end); blank(mask, statement, member_end);
        ++count; i = member_end ? member_end - 1 : member_end; statement = member_end;
      } else {
        blank(out, i, type_end); blank(mask, i, type_end); ++count; i = type_end ? type_end - 1 : type_end;
      }
    }
  }
}
bool is_parameter_list(std::string_view mask, std::size_t open, std::size_t close) {
  auto after = skip_space(mask, close + 1);
  bool declaration_tail = false;
  if (after + 1 < mask.size() && mask[after] == '=' && mask[after + 1] == '>') declaration_tail = true;
  else if (after < mask.size() && (mask[after] == ':' || mask[after] == '{')) declaration_tail = true;
  if (!declaration_tail) return false;

  const auto before = previous_identifier(mask, open);
  if (before == "if" || before == "for" || before == "while" || before == "switch" ||
      before == "catch" || before == "with") return false;
  return true;
}

} // namespace

bool is_typescript_path(const std::string& filename) {
  const auto value = lower(filename);
  return value.size() >= 3 && ((value.size() >= 3 && value.compare(value.size()-3,3,".ts")==0) ||
          (value.size() >= 4 && value.compare(value.size()-4,4,".tsx")==0) ||
          (value.size() >= 4 && value.compare(value.size()-4,4,".mts")==0) ||
          (value.size() >= 4 && value.compare(value.size()-4,4,".cts")==0));
}

TranspileResult transpile_native(const std::string& source, const std::string& filename) {
  TranspileResult result;
  result.typescript = is_typescript_path(filename);
  if (!result.typescript) { result.javascript = source; return result; }
  const auto name = lower(filename);
  result.tsx = name.size() >= 4 && name.compare(name.size()-4,4,".tsx")==0;
  result.javascript = source;
  if (result.tsx) {
    const auto lowered = jsx::lower(source, filename);
    result.javascript = lowered.javascript;
    result.lowered_jsx_elements = lowered.lowered_elements;
  }
  auto mask = code_mask(result.javascript);

  const std::vector<std::string> unsupported{"enum", "namespace"};
  for (const auto& keyword : unsupported) {
    std::size_t pos = 0;
    while ((pos = mask.find(keyword, pos)) != std::string::npos) {
      if (boundary(mask, pos, keyword.size()))
        fail("VENOM-E2502", "Unsupported TypeScript runtime construct", source, filename, pos,
             "The native type-erasure frontend does not lower TypeScript " + keyword + " declarations.",
             "Replace the construct with ordinary JavaScript or precompile it before Venom.");
      pos += keyword.size();
    }
  }

  // Erase ambient declarations (`declare const`, `declare function`, `declare global`, etc.).
  {
    std::size_t pos = 0;
    while ((pos = mask.find("declare", pos)) != std::string::npos) {
      if (!boundary(mask, pos, 7)) { pos += 7; continue; }
      auto cursor = skip_space(mask, pos + 7);
      std::size_t end = statement_end(mask, pos);
      if (word_at(mask, cursor, "global") || word_at(mask, cursor, "module") || word_at(mask, cursor, "namespace")) {
        const auto brace = mask.find('{', cursor);
        if (brace != std::string::npos && brace < end) {
          const auto close = matching(mask, brace, '{', '}');
          if (close == std::string::npos)
            fail("VENOM-E2501", "TypeScript transpilation failed", source, filename, pos,
                 "Unterminated ambient declaration.", "Close the declaration block before rebuilding.");
          end = close + 1;
        }
      }
      blank(result.javascript, pos, end); blank(mask, pos, end);
      ++result.erased_type_declarations; pos = end;
    }
  }

  // Erase overload signatures and abstract method declarations that have no runtime body.
  for (const auto& keyword : {std::string("function"), std::string("abstract")}) {
    std::size_t pos = 0;
    while ((pos = mask.find(keyword, pos)) != std::string::npos) {
      if (!boundary(mask, pos, keyword.size())) { pos += keyword.size(); continue; }
      const auto end = statement_end(mask, pos);
      const auto brace = mask.find('{', pos + keyword.size());
      if (end <= mask.size() && (brace == std::string::npos || brace >= end)) {
        blank(result.javascript, pos, end); blank(mask, pos, end);
        ++result.erased_type_declarations; pos = end;
      } else pos += keyword.size();
    }
  }

  // Erase `import type` / `export type` statements.
  for (const auto& prefix : {std::string("import type"), std::string("export type")}) {
    std::size_t pos = 0;
    while ((pos = mask.find(prefix, pos)) != std::string::npos) {
      if (!boundary(mask, pos, prefix.find(' '))) { pos += prefix.size(); continue; }
      const auto end = statement_end(mask, pos);
      blank(result.javascript, pos, end); blank(mask, pos, end); ++result.erased_type_declarations; pos = end;
    }
  }


  // Erase declarations that exist only in the type system.
  for (const auto& keyword : {std::string("interface"), std::string("type")}) {
    std::size_t pos = 0;
    while ((pos = mask.find(keyword, pos)) != std::string::npos) {
      if (!boundary(mask, pos, keyword.size())) { pos += keyword.size(); continue; }
      std::size_t end = statement_end(mask, pos);
      const auto brace = mask.find('{', pos + keyword.size());
      if (keyword == "interface" && brace != std::string::npos && brace < end) {
        const auto close = matching(mask, brace, '{', '}');
        if (close == std::string::npos) fail("VENOM-E2501", "TypeScript transpilation failed", source, filename, pos,
                                             "Unterminated interface declaration.", "Close the interface declaration before rebuilding.");
        end = close + 1;
        if (end < mask.size() && mask[end] == ';') ++end;
      }
      blank(result.javascript, pos, end); blank(mask, pos, end); ++result.erased_type_declarations; pos = end;
    }
  }

  // Remove declaration-only modifiers without touching runtime properties such as value.protected.
  for (const auto& keyword : {"public", "private", "protected", "readonly", "abstract", "override", "declare"}) {
    std::size_t pos = 0;
    while ((pos = mask.find(keyword, pos)) != std::string::npos) {
      const auto len = std::char_traits<char>::length(keyword);
      if (!boundary(mask, pos, len)) { pos += len; continue; }
      std::size_t next = pos + len;
      const bool separated = next < mask.size() && std::isspace(static_cast<unsigned char>(mask[next])) != 0;
      next = skip_space(mask, next);
      std::size_t previous = pos;
      while (previous > 0 && std::isspace(static_cast<unsigned char>(mask[previous - 1])) != 0) --previous;
      const bool declaration_context = previous == 0 || std::string_view("{;(,}").find(mask[previous - 1]) != std::string_view::npos;
      if (separated && declaration_context && next < mask.size() && ident_start(mask[next])) {
        blank(result.javascript, pos, pos + len);
        blank(mask, pos, pos + len);
      }
      pos += len;
    }
  }

  // Erase generic declarations on functions and classes, plus class `implements` clauses.
  for (const auto& keyword : {std::string("function"), std::string("class")}) {
    std::size_t pos = 0;
    while ((pos = mask.find(keyword, pos)) != std::string::npos) {
      if (!boundary(mask, pos, keyword.size())) { pos += keyword.size(); continue; }
      auto declaration_name = skip_space(mask, pos + keyword.size());
      if (declaration_name < mask.size() && ident_start(mask[declaration_name])) {
        while (declaration_name < mask.size() && ident_char(mask[declaration_name])) ++declaration_name;
        erase_generic_after_identifier(result.javascript, mask, declaration_name, result.erased_annotations);
      }
      if (keyword == "class") {
        const auto body = mask.find('{', declaration_name);
        auto extends_pos = mask.find("extends", declaration_name);
        if (extends_pos != std::string::npos && body != std::string::npos && extends_pos < body && boundary(mask, extends_pos, 7)) {
          auto base = skip_space(mask, extends_pos + 7);
          while (base < body && (ident_char(mask[base]) || mask[base] == '.')) ++base;
          erase_generic_after_identifier(result.javascript, mask, base, result.erased_annotations);
        }
        auto impl = mask.find("implements", declaration_name);
        if (impl != std::string::npos && body != std::string::npos && impl < body && boundary(mask, impl, 10)) {
          blank(result.javascript, impl, body); blank(mask, impl, body); ++result.erased_annotations;
        }
        if (body != std::string::npos) {
          const auto close = matching(mask, body, '{', '}');
          if (close != std::string::npos)
            erase_class_field_annotations(result.javascript, mask, body, close, result.erased_annotations);
        }
      }
      pos = declaration_name;
    }
  }

  // Function and method parameter/return annotations.
  for (std::size_t i = 0; i < mask.size(); ++i) {
    if (mask[i] != '(') continue;
    const auto close = matching(mask, i, '(', ')');
    if (close == std::string::npos) continue;
    if (!is_parameter_list(mask, i, close)) { i = close; continue; }
    erase_type_annotations(result.javascript, mask, i + 1, close, result.erased_annotations);
    auto after = skip_space(mask, close + 1);
    if (after < mask.size() && mask[after] == ':') {
      const auto type_end = consume_type(mask, skip_space(mask, after + 1), "{=;\n");
      blank(result.javascript, after, type_end); blank(mask, after, type_end); ++result.erased_annotations;
    }
    i = close;
  }

  // Variable and class-field annotations.
  for (const auto& keyword : {"const", "let", "var"}) {
    std::size_t pos = 0;
    while ((pos = mask.find(keyword, pos)) != std::string::npos) {
      const auto len = std::char_traits<char>::length(keyword);
      if (!boundary(mask, pos, len)) { pos += len; continue; }
      const auto end = statement_end(mask, pos);
      std::size_t annotation_end = end;
      int paren = 0, bracket = 0, brace = 0, angle = 0;
      for (std::size_t j = pos + len; j < end; ++j) {
        const char c = mask[j];
        if (c == '(') ++paren; else if (c == ')' && paren) --paren;
        else if (c == '[') ++bracket; else if (c == ']' && bracket) --bracket;
        else if (c == '{') ++brace; else if (c == '}' && brace) --brace;
        else if (c == '<') ++angle; else if (c == '>' && angle) --angle;
        else if (c == '=' && !paren && !bracket && !brace && !angle) { annotation_end = j; break; }
      }
      erase_type_annotations(result.javascript, mask, pos + len, annotation_end, result.erased_annotations);
      pos = end;
    }
  }

  // Erase TypeScript type arguments on calls and generic function declarations.
  // Examples: querySelector<HTMLElement>(...), identity<T>(...), new Box<Value>(...).
  for (std::size_t i = 0; i < mask.size(); ++i) {
    if (mask[i] != '<') continue;
    std::size_t previous = i;
    while (previous > 0 && std::isspace(static_cast<unsigned char>(mask[previous - 1])) != 0) --previous;
    if (previous == 0) continue;
    const char before = mask[previous - 1];
    if (!(ident_char(before) || before == ')' || before == ']' || before == '.')) continue;
    const auto close = matching(mask, i, '<', '>');
    if (close == std::string::npos) continue;
    const auto after = skip_space(mask, close + 1);
    if (after >= mask.size() || mask[after] != '(') continue;

    bool type_like = true;
    int nested_paren = 0, nested_bracket = 0, nested_brace = 0;
    for (std::size_t j = i + 1; j < close; ++j) {
      const char c = mask[j];
      if (c == '(') ++nested_paren;
      else if (c == ')' && nested_paren) --nested_paren;
      else if (c == '[') ++nested_bracket;
      else if (c == ']' && nested_bracket) --nested_bracket;
      else if (c == '{') ++nested_brace;
      else if (c == '}' && nested_brace) --nested_brace;
      else if (!std::isspace(static_cast<unsigned char>(c)) && !ident_char(c) &&
               std::string_view(".,|&?:=<>\"'`[]{}()/-").find(c) == std::string_view::npos) {
        type_like = false;
        break;
      }
    }
    if (!type_like || nested_paren || nested_bracket || nested_brace) continue;
    blank(result.javascript, i, close + 1);
    blank(mask, i, close + 1);
    ++result.erased_annotations;
    i = close;
  }

  // `as Type` and `satisfies Type` assertions.
  for (const auto& keyword : {std::string("as"), std::string("satisfies")}) {
    std::size_t pos = 0;
    while ((pos = mask.find(keyword, pos)) != std::string::npos) {
      if (!boundary(mask, pos, keyword.size())) { pos += keyword.size(); continue; }
      const auto begin = skip_space(mask, pos + keyword.size());
      const auto end = consume_type(mask, begin, ",);]}\n");
      if (end > begin) { blank(result.javascript, pos, end); blank(mask, pos, end); ++result.erased_assertions; pos = end; }
      else pos += keyword.size();
    }
  }

  // Non-null assertions (`value!`) where `!` is not an operator pair.
  for (std::size_t i = 1; i + 1 < mask.size(); ++i) {
    if (mask[i] != '!') continue;
    if ((ident_char(mask[i - 1]) || mask[i - 1] == ')' || mask[i - 1] == ']') && mask[i + 1] != '=' && mask[i + 1] != '!') {
      blank(result.javascript, i, i + 1); mask[i] = ' '; ++result.erased_assertions;
    }
  }

  return result;
}


namespace {

std::string read_file_text(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) raise_error("VENOM-E2000", "unable to read structural TypeScript frontend output: " + path.string());
  return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void write_file_text(const std::filesystem::path& path, const std::string& text) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) raise_error("VENOM-E2000", "unable to create structural TypeScript frontend input: " + path.string());
  output.write(text.data(), static_cast<std::streamsize>(text.size()));
}

std::string embedded_cache_key(const std::string& source, const std::string& filename,
                               const std::string& compiler_version) {
  const std::string material =
      "venom-typescript-embedded-cache-v3\nfrontend=embedded-quickjs-typescript\nversion=" +
      compiler_version + "\nbridge=1\njsx=venom-classic-v1\nfilename=" + filename + "\nsource=" + source;
  return venom::package::sha256_hex(reinterpret_cast<const unsigned char*>(material.data()), material.size());
}

bool read_embedded_cache(const std::filesystem::path& base, TranspileResult& result) {
  const auto js = base.string() + ".js";
  const auto meta = base.string() + ".meta";
  if (!std::filesystem::is_regular_file(js) || !std::filesystem::is_regular_file(meta)) return false;
  const auto metadata = read_file_text(meta);
  if (metadata.find("schema=3\nfrontend=embedded-quickjs-typescript\n") != 0) return false;
  result.javascript = read_file_text(js);
  result.source_map = std::filesystem::is_regular_file(base.string() + ".map")
      ? read_file_text(base.string() + ".map") : std::string{};
  result.frontend = "embedded-quickjs-typescript";
  {
    const auto version = metadata.find("version=");
    if (version != std::string::npos) {
      const auto begin = version + 8;
      const auto end = metadata.find('\n', begin);
      result.frontend_version = metadata.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    }
  }
  result.typescript = true;
  result.tsx = metadata.find("tsx=1\n") != std::string::npos;
  const auto jsx = metadata.find("jsx=");
  if (jsx != std::string::npos) result.lowered_jsx_elements = static_cast<std::size_t>(std::stoull(metadata.substr(jsx + 4)));
  return !result.javascript.empty();
}

void write_embedded_cache(const std::filesystem::path& base, const TranspileResult& result) {
  std::filesystem::create_directories(base.parent_path());
  write_file_text(base.string() + ".js", result.javascript);
  if (!result.source_map.empty()) write_file_text(base.string() + ".map", result.source_map);
  std::ostringstream meta;
  meta << "schema=3\nfrontend=embedded-quickjs-typescript\nversion=" << result.frontend_version
       << "\nbridge=1\ntsx=" << (result.tsx ? 1 : 0) << "\ndiagnostics=0\njsx="
       << result.lowered_jsx_elements << "\n";
  write_file_text(base.string() + ".meta", meta.str());
}

void raise_embedded_diagnostics(const TranspileResult& result, const std::string& source,
                                const std::string& filename) {
  for (const auto& diagnostic : result.diagnostics) {
    if (diagnostic.category != DiagnosticCategory::Error) continue;
    SourceLocation location;
    location.file = filename;
    location.line = diagnostic.line ? diagnostic.line : 1;
    location.column = diagnostic.column ? diagnostic.column - 1 : 0;
    std::size_t line_begin = 0;
    for (std::size_t line = 1; line < location.line && line_begin < source.size(); ++line) {
      const auto newline = source.find('\n', line_begin);
      if (newline == std::string::npos) { line_begin = source.size(); break; }
      line_begin = newline + 1;
    }
    const auto line_end = source.find('\n', line_begin);
    location.source_line = source.substr(line_begin, line_end == std::string::npos ? std::string::npos : line_end - line_begin);
    raise_diagnostic("VENOM-E2505", "TypeScript compilation failed", std::move(location),
                     "TS" + std::to_string(diagnostic.code) + ": " + diagnostic.message,
                     "Correct the TypeScript source before rebuilding.",
                     "docs/reference/diagnostics.md#venom-e2505");
  }
}

TranspileResult transpile_embedded(const std::string& source, const std::string& filename) {
  auto& service = EmbeddedFrontendService::instance();
  const auto compiler_version = service.compiler_version();
  std::filesystem::path cache_base;
  bool cache_enabled = false;
  bool cache_verbose = false;
  {
    std::lock_guard<std::mutex> lock(g_cache_mutex);
    cache_enabled = g_cache_enabled && !g_cache_directory.empty();
    cache_verbose = g_cache_verbose;
    if (cache_enabled) {
      const auto digest = embedded_cache_key(source, filename, compiler_version);
      cache_base = g_cache_directory / digest.substr(0, 2) / digest;
    }
  }
  if (cache_enabled) {
    TranspileResult cached;
    if (read_embedded_cache(cache_base, cached)) {
      ++g_cache_hits;
      if (cache_verbose) std::cerr << "[CACHE]    Embedded TypeScript frontend hit: " << filename << "\n";
      return cached;
    }
    ++g_cache_misses;
    if (cache_verbose) std::cerr << "[CACHE]    Embedded TypeScript frontend miss: " << filename << "\n";
  }

  auto result = service.transpile(source, filename);
  raise_embedded_diagnostics(result, source, filename);
  // TypeScript preserves JSX so Venom's deterministic classic JSX lowering and
  // local runtime contract remain authoritative.
  if (result.tsx) {
    const auto lowered = jsx::lower(result.javascript, filename);
    result.javascript = lowered.javascript;
    result.lowered_jsx_elements = lowered.lowered_elements;
  }
  // Cache only clean emissions. Diagnostic-bearing results stay live so full
  // structured diagnostics are never lost through the compact cache format.
  if (cache_enabled && result.diagnostics.empty()) {
    std::lock_guard<std::mutex> lock(g_cache_mutex);
    write_embedded_cache(cache_base, result);
    ++g_cache_writes;
  }
  return result;
}

} // namespace

void configure_cache(bool enabled, const std::filesystem::path& directory, bool verbose) {
  std::lock_guard<std::mutex> lock(g_cache_mutex);
  g_cache_enabled = enabled;
  g_cache_verbose = verbose;
  g_cache_directory = directory;
  if (enabled && !directory.empty()) std::filesystem::create_directories(directory);
}

CacheStats cache_stats() { return {g_cache_hits.load(), g_cache_misses.load(), g_cache_writes.load()}; }
void reset_cache_stats() { g_cache_hits = 0; g_cache_misses = 0; g_cache_writes = 0; }

TranspileResult transpile(const std::string& source, const std::string& filename) {
  if (!is_typescript_path(filename)) {
    TranspileResult result;
    result.javascript = source;
    return result;
  }
  const char* selected = std::getenv("VENOM_TYPESCRIPT_FRONTEND");
  if (selected) {
    const std::string frontend(selected);
    if (frontend == "native") {
      auto result = transpile_native(source, filename);
      result.frontend = "venom-native-typescript-eraser";
      result.frontend_version = "4";
      return result;
    }
    if (frontend == "embedded" || frontend == "quickjs") {
      return transpile_embedded(source, filename);
    }
    if (frontend == "node" || frontend == "structural") {
      raise_error("VENOM-E2000", 
          "VENOM_TYPESCRIPT_FRONTEND=" + frontend +
          " is no longer supported; Venom's TypeScript frontend is embedded and does not require Node.js");
    }
    raise_error("VENOM-E2000", "unknown VENOM_TYPESCRIPT_FRONTEND value: " + frontend);
  }
  return transpile_embedded(source, filename);
}

const char* frontend_name() { return "embedded-quickjs-typescript"; }
const char* frontend_version() {
  static const std::string version = EmbeddedFrontendService::instance().compiler_version();
  return version.c_str();
}

} // namespace venom::compiler::typescript
