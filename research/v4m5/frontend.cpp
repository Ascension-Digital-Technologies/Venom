#include "frontend.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace venom::v4m5 {
namespace {
using v4m3::Instruction;
using v4m3::Op;
using v4m3::Program;

struct Token {
  std::string text;
  std::size_t pos = 0;
  bool number = false;
};

[[noreturn]] void fail(const std::string& why, std::size_t pos) {
  throw std::runtime_error("V4-M5 JS subset at byte " + std::to_string(pos) + ": " + why);
}

std::vector<Token> tokenize(const std::string& source) {
  if (source.size() > 65536) fail("source length limit exceeded", 0);
  std::vector<Token> out;
  std::size_t i = 0;
  while (i < source.size()) {
    const unsigned char ch = static_cast<unsigned char>(source[i]);
    if (std::isspace(ch)) { ++i; continue; }
    if (source.compare(i, 2, "//") == 0) {
      i += 2;
      while (i < source.size() && source[i] != '\n') ++i;
      continue;
    }
    if (source.compare(i, 2, "/*") == 0) {
      const auto close = source.find("*/", i + 2);
      if (close == std::string::npos) fail("unterminated comment", i);
      i = close + 2;
      continue;
    }
    const std::size_t begin = i;
    if (std::isalpha(ch) || ch == '_') {
      while (i < source.size()) {
        const unsigned char c = static_cast<unsigned char>(source[i]);
        if (!(std::isalnum(c) || c == '_')) break;
        ++i;
      }
      out.push_back({source.substr(begin, i - begin), begin, false});
    } else if (std::isdigit(ch)) {
      while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i]))) ++i;
      if (i < source.size() && (source[i] == '.' || std::isalpha(static_cast<unsigned char>(source[i])) || source[i] == '_'))
        fail("only decimal integer literals are supported", begin);
      out.push_back({source.substr(begin, i - begin), begin, true});
    } else {
      const std::string triples[] = {"===", "!=="};
      bool found = false;
      for (const auto& op : triples) {
        if (source.compare(i, op.size(), op) == 0) {
          out.push_back({op, begin, false}); i += op.size(); found = true; break;
        }
      }
      if (!found) {
        for (const auto& op : {"<=", ">=", "==", "!=", "&&", "||", "=>", "++", "--"}) {
          const auto len = std::char_traits<char>::length(op);
          if (source.compare(i, len, op) == 0) {
            out.push_back({op, begin, false}); i += len; found = true; break;
          }
        }
      }
      if (!found) {
        if (std::string("(){};,=+-*^!<>").find(source[i]) == std::string::npos)
          fail("unsupported character", begin);
        out.push_back({source.substr(i, 1), i, false});
        ++i;
      }
    }
    if (out.size() > 8192) fail("token limit exceeded", begin);
  }
  out.push_back({"<eof>", source.size(), false});
  return out;
}

enum class Type : std::uint8_t { Number, Boolean };
struct Temporary { std::uint8_t reg; Type type; };
struct Binding { std::uint8_t reg; Type type; bool mutable_value; };

class Parser {
 public:
  explicit Parser(const std::string& source) : tokens_(tokenize(source)) {
    used_[0] = true; // Register 0 always stores truthy 1 for unconditional jumps.
    code_.push_back({Op::Const, 0, 0, 1});
  }
  Program compile() {
    expect("function"); expect("main"); expect("("); expect(")");
    const bool returns = block();
    if (!returns) fail("function can exit without return", here().pos);
    if (here().text != "<eof>") fail("only one main() function is supported", here().pos);
    code_.push_back({Op::Halt, 0, 0, 0}); // Unreachable verifier sentinel.
    if (code_.size() > 65536) fail("VM instruction limit exceeded", here().pos);
    return code_;
  }
 private:
  const Token& here() const { return tokens_[index_]; }
  bool eat(const char* symbol) {
    if (here().text != symbol) return false;
    ++index_; return true;
  }
  void expect(const char* symbol) {
    if (!eat(symbol)) fail(std::string("expected '") + symbol + "', saw '" + here().text + "'", here().pos);
  }
  Token identifier() {
    if (here().text.empty() || here().text == "<eof>" ||
        !(std::isalpha(static_cast<unsigned char>(here().text[0])) || here().text[0] == '_'))
      fail("identifier expected", here().pos);
    return tokens_[index_++];
  }
  std::uint32_t number() {
    if (!here().number) fail("integer literal expected", here().pos);
    const auto start = here().pos;
    std::uint64_t value = 0;
    for (const char c : here().text) {
      value = value * 10 + static_cast<unsigned>(c - '0');
      if (value > 2147483647ull) fail("decimal integer literal exceeds signed int32", start);
    }
    ++index_;
    return static_cast<std::uint32_t>(value);
  }
  std::uint8_t alloc() {
    for (std::size_t i = 1; i < used_.size(); ++i) {
      if (!used_[i]) { used_[i] = true; return static_cast<std::uint8_t>(i); }
    }
    fail("register budget exhausted (15 locals/temporaries)", here().pos);
  }
  void release(std::uint8_t reg) {
    if (reg == 0 || reg >= used_.size() || !used_[reg])
      throw std::runtime_error("VM frontend register bookkeeping error");
    used_[reg] = false;
  }
  std::size_t emit(Instruction ins) {
    if (code_.size() >= 65535) fail("VM instruction limit exceeded", here().pos);
    code_.push_back(ins); return code_.size() - 1;
  }
  void patch(std::size_t at, std::size_t target) {
    if (target >= 65536) fail("VM target limit exceeded", here().pos);
    code_[at].b = static_cast<std::uint32_t>(target);
  }
  void enter_scope() {
    if (scopes_.size() >= 32) fail("scope nesting limit exceeded", here().pos);
    scopes_.push_back({});
  }
  void leave_scope() {
    for (const auto& name : scopes_.back()) {
      release(locals_.at(name).reg);
      locals_.erase(name);
    }
    scopes_.pop_back();
  }
  Binding& binding(const Token& id) {
    const auto it = locals_.find(id.text);
    if (it == locals_.end()) fail("unknown local '" + id.text + "'", id.pos);
    return it->second;
  }
  static void need(Type actual, Type expected, std::size_t pos) {
    if (actual != expected) fail("static type mismatch in integer-only JS subset", pos);
  }
  Temporary literal(std::uint32_t value, Type type) {
    const auto dst = alloc(); emit({Op::Const, dst, 0, value});
    return {dst, type};
  }
  Temporary binary(Temporary left, Temporary right, const std::string& op, std::size_t pos) {
    Op kind = Op::AddChecked;
    Type result = Type::Number;
    if (op == "+" || op == "-" || op == "*" || op == "^") {
      need(left.type, Type::Number, pos); need(right.type, Type::Number, pos);
      kind = op == "+" ? Op::AddChecked : op == "-" ? Op::SubChecked :
             op == "*" ? Op::MulChecked : Op::Xor;
    } else if (op == "===" || op == "!==") {
      need(left.type, right.type, pos);
      kind = Op::Equal; result = Type::Boolean;
    } else {
      need(left.type, Type::Number, pos); need(right.type, Type::Number, pos);
      kind = op == "<" ? Op::Less : op == "<=" ? Op::LessEqual :
             op == ">" ? Op::Greater : Op::GreaterEqual;
      result = Type::Boolean;
    }
    emit({kind, left.reg, left.reg, right.reg});
    release(right.reg);
    if (op == "!==") {
      auto zero = literal(0, Type::Boolean);
      emit({Op::Equal, left.reg, left.reg, zero.reg});
      release(zero.reg);
    }
    return {left.reg, result};
  }
  Temporary primary() {
    if (eat("(")) {
      auto val = expression(); expect(")"); return val;
    }
    if (here().number) return literal(number(), Type::Number);
    if (eat("true")) return literal(1, Type::Boolean);
    if (eat("false")) return literal(0, Type::Boolean);
    const Token id = identifier();
    if (id.text == "host") {
      expect("(");
      const auto cap = number();
      if (cap == 0) fail("host capability must be nonzero", id.pos);
      expect(","); auto arg = expression(); expect(")");
      need(arg.type, Type::Number, id.pos);
      emit({Op::HostCall, arg.reg, arg.reg, cap});
      return arg;
    }
    if (here().text == "(") fail("general function calls are not supported", id.pos);
    const auto b = binding(id);
    const auto tmp = alloc();
    emit({Op::Move, tmp, b.reg, 0});
    return {tmp, b.type};
  }
  Temporary unary() {
    if (here().text == "-" || here().text == "!" || here().text == "+") {
      if (++unary_depth_ > 64) fail("unary expression depth limit exceeded", here().pos);
      const auto op = tokens_[index_++];
      auto val = unary();
      --unary_depth_;
      if (op.text == "+") { need(val.type, Type::Number, op.pos); return val; }
      if (op.text == "-") need(val.type, Type::Number, op.pos);
      const auto zero = literal(0, op.text == "!" ? val.type : Type::Number);
      emit({op.text == "-" ? Op::SubChecked : Op::Equal,
            val.reg, zero.reg, val.reg});
      release(zero.reg);
      if (op.text == "!") val.type = Type::Boolean;
      return val;
    }
    return primary();
  }
  Temporary multiplicative() {
    auto lhs = unary();
    while (here().text == "*") {
      const auto op = tokens_[index_++]; lhs = binary(lhs, unary(), op.text, op.pos);
    }
    return lhs;
  }
  Temporary additive() {
    auto lhs = multiplicative();
    while (here().text == "+" || here().text == "-") {
      const auto op = tokens_[index_++]; lhs = binary(lhs, multiplicative(), op.text, op.pos);
    }
    return lhs;
  }
  Temporary relational() {
    auto lhs = additive();
    while (here().text == "<" || here().text == "<=" || here().text == ">" || here().text == ">=") {
      const auto op = tokens_[index_++]; lhs = binary(lhs, additive(), op.text, op.pos);
    }
    return lhs;
  }
  Temporary equality() {
    auto lhs = relational();
    while (here().text == "===" || here().text == "!==") {
      const auto op = tokens_[index_++]; lhs = binary(lhs, relational(), op.text, op.pos);
    }
    return lhs;
  }
  Temporary expression() {
    if (++expression_depth_ > 64) fail("expression nesting limit exceeded", here().pos);
    auto lhs = equality();
    while (here().text == "^") {
      const auto op = tokens_[index_++]; lhs = binary(lhs, equality(), op.text, op.pos);
    }
    --expression_depth_;
    return lhs;
  }
  bool block() {
    expect("{"); enter_scope();
    bool guaranteed_return = false;
    while (here().text != "}") {
      if (here().text == "<eof>") fail("unterminated block", here().pos);
      if (guaranteed_return) fail("unreachable statement after unconditional return", here().pos);
      guaranteed_return = statement();
    }
    expect("}"); leave_scope();
    return guaranteed_return;
  }
  bool statement() {
    if (here().text == "{") return block();
    if (eat("let") || eat("const")) {
      const bool mutable_value = tokens_[index_ - 1].text == "let";
      const Token id = identifier();
      if (id.text == "host" || id.text == "main" || id.text == "true" || id.text == "false" ||
          locals_.find(id.text) != locals_.end())
        fail("unsupported or duplicate binding '" + id.text + "'", id.pos);
      const auto dst = alloc();
      expect("="); auto val = expression(); expect(";");
      emit({Op::Move, dst, val.reg, 0}); release(val.reg);
      locals_.emplace(id.text, Binding{dst, val.type, mutable_value});
      scopes_.back().push_back(id.text);
      return false;
    }
    if (eat("return")) {
      auto val = expression(); expect(";");
      emit({Op::Halt, 0, val.reg, 0}); release(val.reg);
      return true;
    }
    if (eat("if")) {
      expect("("); auto cond = expression(); expect(")"); release(cond.reg);
      const auto branch_true = emit({Op::Jnz, 0, cond.reg, 0});
      const auto branch_false = emit({Op::Jnz, 0, 0, 0});
      patch(branch_true, code_.size());
      const bool true_returns = block();
      std::size_t skip_false = 0;
      if (!true_returns) skip_false = emit({Op::Jnz, 0, 0, 0});
      patch(branch_false, code_.size());
      bool false_returns = false;
      if (eat("else")) false_returns = block();
      if (!true_returns) patch(skip_false, code_.size());
      return true_returns && false_returns;
    }
    if (eat("while")) {
      const std::size_t header = code_.size();
      expect("("); auto cond = expression(); expect(")"); release(cond.reg);
      const auto branch_true = emit({Op::Jnz, 0, cond.reg, 0});
      const auto branch_end = emit({Op::Jnz, 0, 0, 0});
      patch(branch_true, code_.size());
      const bool body_returns = block();
      if (!body_returns) emit({Op::Jnz, 0, 0, static_cast<std::uint32_t>(header)});
      patch(branch_end, code_.size());
      return false; // The loop may execute zero times.
    }
    if (eat(";")) return false;
    const Token id = identifier();
    if (id.text == "host") fail("host calls may occur only as expressions", id.pos);
    Binding& dest = binding(id);
    if (!dest.mutable_value) fail("cannot assign to const binding", id.pos);
    expect("="); auto val = expression(); expect(";");
    need(val.type, dest.type, id.pos);
    emit({Op::Move, dest.reg, val.reg, 0});
    release(val.reg);
    return false;
  }

  std::vector<Token> tokens_;
  std::size_t index_ = 0;
  std::size_t expression_depth_ = 0;
  std::size_t unary_depth_ = 0;
  Program code_;
  std::array<bool,16> used_{};
  std::unordered_map<std::string,Binding> locals_;
  std::vector<std::vector<std::string>> scopes_;
};
} // namespace

v4m3::Program compile_source(const std::string& source) {
  Parser parser(source);
  return parser.compile();
}
} // namespace venom::v4m5
