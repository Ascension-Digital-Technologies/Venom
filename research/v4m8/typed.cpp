#include "typed.hpp"

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

namespace venom::v4m8 {
namespace {
using v4m3::Op;
using v4m3::Program;
constexpr std::size_t kMaxSource = 65536;
constexpr std::size_t kMaxTokens = 8192;
constexpr std::size_t kMaxNodes = 8192;
void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
[[noreturn]] void syntax(const char* message, std::size_t pos) {
  throw std::runtime_error(std::string("typed SSA source at byte ") +
                           std::to_string(pos) + ": " + message);
}
struct Token {
  std::string text;
  std::size_t pos = 0;
  bool number = false;
};
std::vector<Token> tokenize(const std::string& source) {
  require(source.size() <= kMaxSource, "typed SSA source limit exceeded");
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
      const auto end = source.find("*/", i + 2);
      if (end == std::string::npos) syntax("unterminated comment", i);
      i = end + 2;
      continue;
    }
    const auto begin = i;
    if (std::isalpha(ch) || ch == '_') {
      do { ++i; } while (i < source.size() &&
          (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_'));
      out.push_back({source.substr(begin, i - begin), begin, false});
    } else if (std::isdigit(ch)) {
      do { ++i; } while (i < source.size() &&
          std::isdigit(static_cast<unsigned char>(source[i])));
      if (i < source.size() &&
          (source[i] == '.' || source[i] == '_' ||
           std::isalpha(static_cast<unsigned char>(source[i]))))
        syntax("only signed-int32 decimal literals supported", begin);
      out.push_back({source.substr(begin, i - begin), begin, true});
    } else {
      bool found = false;
      for (const char* operator_text : {"===", "!==", "<=", ">=", "&&", "||", "==", "!=", "++", "--"}) {
        const std::string op(operator_text);
        if (source.compare(i, op.size(), op) == 0) {
          out.push_back({op, i, false});
          i += op.size();
          found = true;
          break;
        }
      }
      if (!found) {
        if (std::string("(){};,=+-*^!<>").find(source[i]) == std::string::npos)
          syntax("unsupported character", i);
        out.push_back({source.substr(i, 1), i, false});
        ++i;
      }
    }
    if (out.size() > kMaxTokens) syntax("token limit exceeded", begin);
  }
  out.push_back({"<eof>", source.size(), false});
  return out;
}
bool is_reserved(const std::string& name) {
  for (const char* word : {"function", "return", "const", "let", "var", "if",
       "else", "while", "for", "true", "false", "switch", "break", "continue",
       "do", "new", "class", "this", "typeof", "void", "delete", "null",
       "undefined", "await", "yield", "import", "export", "try", "catch", "throw"}) {
    if (name == word) return true;
  }
  return false;
}
bool binary(Kind kind) {
  return kind != Kind::Literal && kind != Kind::HostCall;
}
bool number_op(Kind kind) {
  return kind == Kind::Add || kind == Kind::Sub || kind == Kind::Mul ||
         kind == Kind::Xor || kind == Kind::Less || kind == Kind::LessEqual ||
         kind == Kind::Greater || kind == Kind::GreaterEqual;
}
Type output_type(Kind kind) {
  switch (kind) {
    case Kind::Literal: case Kind::Add: case Kind::Sub:
    case Kind::Mul: case Kind::Xor: case Kind::HostCall: return Type::Number;
    default: return Type::Boolean;
  }
}
Op vm_op(Kind kind) {
  switch (kind) {
    case Kind::Add: return Op::AddChecked;
    case Kind::Sub: return Op::SubChecked;
    case Kind::Mul: return Op::MulChecked;
    case Kind::Xor: return Op::Xor;
    case Kind::Equal: return Op::Equal;
    case Kind::Less: return Op::Less;
    case Kind::LessEqual: return Op::LessEqual;
    case Kind::Greater: return Op::Greater;
    case Kind::GreaterEqual: return Op::GreaterEqual;
    case Kind::HostCall: return Op::HostCall;
    case Kind::Literal: return Op::Const;
    default: throw std::runtime_error("unknown typed SSA operation");
  }
}
std::int64_t signed32(std::uint32_t raw) {
  return raw <= 0x7fffffffu ? std::int64_t(raw) : std::int64_t(raw) - 0x100000000ll;
}
std::uint32_t checked(std::int64_t v) {
  require(v >= -0x80000000ll && v <= 0x7fffffffll,
          "typed SSA int32 overflow");
  return static_cast<std::uint32_t>(v);
}
struct Value { std::uint32_t id; Type type; };
struct Binding { Value value; bool mutable_value; };
class Parser {
 public:
  explicit Parser(const std::string& source) : tokens_(tokenize(source)) {}
  Module parse() {
    expect("function"); expect("main"); expect("("); expect(")"); expect("{");
    while (!eat("return")) {
      if (here().text == "}" || here().text == "<eof>")
        syntax("main requires an explicit return", here().pos);
      statement();
    }
    const auto result = expression();
    expect(";"); expect("}");
    require(here().text == "<eof>", "extra statements or unsupported function after main");
    module_.result = result.id;
    module_.result_type = result.type;
    verify(module_);
    return module_;
  }
 private:
  const Token& here() const { return tokens_[pos_]; }
  bool eat(const char* text) {
    if (here().text != text) return false;
    ++pos_; return true;
  }
  void expect(const char* text) {
    if (!eat(text)) syntax("unexpected or unsupported token", here().pos);
  }
  Token identifier() {
    const auto token = here();
    if (token.text.empty() || !(std::isalpha(static_cast<unsigned char>(token.text[0])) ||
        token.text[0] == '_') || is_reserved(token.text))
      syntax("identifier expected (reserved keyword rejected)", token.pos);
    ++pos_;
    return token;
  }
  std::uint32_t integer() {
    if (!here().number) syntax("integer literal required", here().pos);
    std::uint64_t value = 0;
    for (const char c : here().text) {
      value = value * 10 + static_cast<unsigned>(c - '0');
      if (value > 2147483647ull) syntax("integer literal over int32 range", here().pos);
    }
    ++pos_;
    return static_cast<std::uint32_t>(value);
  }
  Value node(Kind kind, Type type, std::uint32_t a, std::uint32_t b,
             std::uint32_t immediate = 0) {
    if (module_.nodes.size() >= kMaxNodes)
      syntax("typed SSA node budget exceeded", here().pos);
    module_.nodes.push_back({kind, type, a, b, immediate});
    return {static_cast<std::uint32_t>(module_.nodes.size()), type};
  }
  Value literal(std::uint32_t value, Type type) {
    return node(Kind::Literal, type, 0, 0, value);
  }
  void expect_type(Type got, Type wanted, std::size_t at) {
    if (got != wanted) syntax("static type mismatch (no JS coercion)", at);
  }
  Value combine(Value left, Value right, const std::string& op, std::size_t at) {
    if (op == "===" || op == "!==") {
      expect_type(right.type, left.type, at);
      auto equal = node(Kind::Equal, Type::Boolean, left.id, right.id);
      if (op == "!==") {
        const auto zero = literal(0, Type::Boolean);
        return node(Kind::Equal, Type::Boolean, equal.id, zero.id);
      }
      return equal;
    }
    expect_type(left.type, Type::Number, at);
    expect_type(right.type, Type::Number, at);
    Kind kind;
    if (op == "+") kind = Kind::Add;
    else if (op == "-") kind = Kind::Sub;
    else if (op == "*") kind = Kind::Mul;
    else if (op == "^") kind = Kind::Xor;
    else if (op == "<") kind = Kind::Less;
    else if (op == "<=") kind = Kind::LessEqual;
    else if (op == ">") kind = Kind::Greater;
    else if (op == ">=") kind = Kind::GreaterEqual;
    else syntax("unsupported binary operator", at);
    return node(kind, output_type(kind), left.id, right.id);
  }
  Value primary() {
    if (eat("(")) { auto value = expression(); expect(")"); return value; }
    if (here().number) return literal(integer(), Type::Number);
    if (eat("true")) return literal(1, Type::Boolean);
    if (eat("false")) return literal(0, Type::Boolean);
    const auto id = identifier();
    if (id.text == "host") {
      expect("("); const auto cap = integer();
      if (cap == 0) syntax("zero host capability", id.pos);
      expect(","); auto arg = expression(); expect(")");
      expect_type(arg.type, Type::Number, id.pos);
      return node(Kind::HostCall, Type::Number, arg.id, 0, cap);
    }
    if (here().text == "(") syntax("arbitrary JS function calls unsupported", id.pos);
    const auto it = locals_.find(id.text);
    if (it == locals_.end()) syntax("undefined local variable", id.pos);
    return it->second.value;
  }
  Value unary() {
    if (here().text == "-" || here().text == "+" || here().text == "!") {
      if (++unary_depth_ > 64) syntax("unary recursion limit", here().pos);
      const auto operator_token = tokens_[pos_++];
      const auto value = unary();
      --unary_depth_;
      if (operator_token.text == "+") {
        expect_type(value.type, Type::Number, operator_token.pos);
        return value;
      }
      if (operator_token.text == "-") {
        expect_type(value.type, Type::Number, operator_token.pos);
        const auto zero = literal(0, Type::Number);
        return node(Kind::Sub, Type::Number, zero.id, value.id);
      }
      expect_type(value.type, Type::Boolean, operator_token.pos);
      const auto zero = literal(0, Type::Boolean);
      return node(Kind::Equal, Type::Boolean, value.id, zero.id);
    }
    return primary();
  }
  Value product() {
    auto a = unary();
    while (here().text == "*") {
      const auto op = tokens_[pos_++]; a = combine(a, unary(), op.text, op.pos);
    }
    return a;
  }
  Value sum() {
    auto a = product();
    while (here().text == "+" || here().text == "-") {
      const auto op = tokens_[pos_++]; a = combine(a, product(), op.text, op.pos);
    }
    return a;
  }
  Value relational() {
    auto a = sum();
    while (here().text == "<" || here().text == "<=" ||
           here().text == ">" || here().text == ">=") {
      const auto op = tokens_[pos_++]; a = combine(a, sum(), op.text, op.pos);
    }
    return a;
  }
  Value equality() {
    auto a = relational();
    while (here().text == "===" || here().text == "!==") {
      const auto op = tokens_[pos_++]; a = combine(a, relational(), op.text, op.pos);
    }
    return a;
  }
  Value xor_expr() {
    auto a = equality();
    while (here().text == "^") {
      const auto op = tokens_[pos_++]; a = combine(a, equality(), op.text, op.pos);
    }
    return a;
  }
  Value expression() {
    if (++expression_depth_ > 64) syntax("expression nesting limit", here().pos);
    const auto a = xor_expr();
    --expression_depth_;
    return a;
  }
  void statement() {
    if (eat(";")) return;
    if (eat("let") || eat("const")) {
      const bool mutable_value = tokens_[pos_ - 1].text == "let";
      const auto id = identifier();
      if (id.text == "main" || id.text == "host" || locals_.count(id.text))
        syntax("reserved or duplicate local binding", id.pos);
      expect("=");
      const auto value = expression();
      expect(";");
      locals_.emplace(id.text, Binding{value, mutable_value});
      return;
    }
    const auto id = identifier();
    auto it = locals_.find(id.text);
    if (it == locals_.end()) syntax("assignment to unknown local", id.pos);
    if (!it->second.mutable_value) syntax("assignment to const rejected", id.pos);
    expect("=");
    const auto value = expression();
    expect(";");
    expect_type(value.type, it->second.value.type, id.pos);
    it->second.value = value; // new immutable SSA binding version
  }
  std::vector<Token> tokens_;
  std::size_t pos_ = 0;
  std::size_t expression_depth_ = 0;
  std::size_t unary_depth_ = 0;
  Module module_;
  std::unordered_map<std::string, Binding> locals_;
};
} // namespace

void verify(const Module& m) {
  require(!m.nodes.empty() && m.nodes.size() <= kMaxNodes,
          "typed SSA invalid node count");
  require(m.result > 0 && m.result <= m.nodes.size(),
          "typed SSA invalid return value");
  std::vector<Type> types(m.nodes.size() + 1, Type::Number);
  for (std::size_t i = 0; i < m.nodes.size(); ++i) {
    const auto& n = m.nodes[i];
    const auto id = static_cast<std::uint32_t>(i + 1);
    const auto check_operand = [&](std::uint32_t value) {
      require(value > 0 && value < id, "typed SSA undefined or forward operand");
    };
    switch (n.kind) {
      case Kind::Literal:
        require(n.a == 0 && n.b == 0, "typed SSA bad literal operands");
        if (n.type == Type::Boolean) require(n.immediate <= 1, "typed SSA invalid Boolean");
        else require(n.type == Type::Number, "typed SSA invalid type");
        break;
      case Kind::HostCall:
        require(n.type == Type::Number && n.b == 0 && n.immediate != 0,
                "typed SSA invalid host call");
        check_operand(n.a);
        require(types[n.a] == Type::Number, "typed SSA host argument not Number");
        break;
      case Kind::Equal:
        require(n.type == Type::Boolean && n.immediate == 0,
                "typed SSA invalid equality");
        check_operand(n.a); check_operand(n.b);
        require(types[n.a] == types[n.b], "typed SSA equality type mismatch");
        break;
      case Kind::Add: case Kind::Sub: case Kind::Mul: case Kind::Xor:
      case Kind::Less: case Kind::LessEqual: case Kind::Greater: case Kind::GreaterEqual:
        require(n.immediate == 0 && n.type == output_type(n.kind),
                "typed SSA invalid numeric operation");
        check_operand(n.a); check_operand(n.b);
        require(types[n.a] == Type::Number && types[n.b] == Type::Number,
                "typed SSA numeric operand not Number");
        break;
      default:
        throw std::runtime_error("typed SSA unknown operation kind");
    }
    types[id] = n.type;
  }
  require(m.result_type == types[m.result], "typed SSA return type mismatch");
}

Module parse_typed(const std::string& source) {
  Parser parser(source);
  return parser.parse();
}

v4m3::Program lower(const Module& m) {
  verify(m);
  const auto count = m.nodes.size();
  std::vector<std::uint32_t> uses(count + 1, 0);
  for (const auto& n : m.nodes) {
    if (n.kind == Kind::HostCall || binary(n.kind)) ++uses[n.a];
    if (binary(n.kind)) ++uses[n.b];
  }
  ++uses[m.result];
  std::array<bool, 16> busy{};
  busy[0] = true; // r0 holds one; never available for allocation.
  std::vector<std::uint8_t> physical(count + 1, 0);
  Program code{{Op::Const, 0, 0, 1}};
  for (std::size_t i = 0; i < count; ++i) {
    const auto& n = m.nodes[i];
    const auto id = static_cast<std::uint32_t>(i + 1);
    const auto a_reg = n.a ? physical[n.a] : std::uint8_t{0};
    const auto b_reg = n.b ? physical[n.b] : std::uint8_t{0};
    const auto consume = [&](std::uint32_t value) {
      require(uses[value] > 0, "typed SSA invalid last use");
      if (--uses[value] == 0) {
        require(physical[value] != 0, "typed SSA allocator lost value");
        busy[physical[value]] = false;
      }
    };
    if (n.kind == Kind::HostCall || binary(n.kind)) consume(n.a);
    if (binary(n.kind)) consume(n.b);
    std::uint8_t dst = 0;
    for (std::uint8_t reg = 1; reg < 16; ++reg) if (!busy[reg]) {
      dst = reg; break;
    }
    require(dst != 0, "typed SSA live-register pressure exceeds 15 (spilling pending)");
    busy[dst] = true;
    physical[id] = dst;
    const auto kind = vm_op(n.kind);
    if (kind == Op::Const) code.push_back({kind, dst, 0, n.immediate});
    else if (kind == Op::HostCall) code.push_back({kind, dst, a_reg, n.immediate});
    else code.push_back({kind, dst, a_reg, b_reg});
    if (uses[id] == 0) busy[dst] = false;
  }
  require(physical[m.result] != 0, "typed SSA missing return register");
  code.push_back({Op::Halt, 0, physical[m.result], 0});
  require(code.size() <= 65536, "typed SSA bytecode instruction limit");
  return code;
}

std::uint32_t interpret(const Module& m, const v4m3::HostPolicy& host) {
  verify(m);
  std::vector<std::uint32_t> values(m.nodes.size() + 1, 0);
  std::uint64_t quota = host.max_calls;
  for (std::size_t i = 0; i < m.nodes.size(); ++i) {
    const auto& n = m.nodes[i];
    const auto a = values[n.a], b = values[n.b];
    std::uint32_t result = 0;
    switch (n.kind) {
      case Kind::Literal: result = n.immediate; break;
      case Kind::Add: result = checked(signed32(a) + signed32(b)); break;
      case Kind::Sub: result = checked(signed32(a) - signed32(b)); break;
      case Kind::Mul: result = checked(signed32(a) * signed32(b)); break;
      case Kind::Xor: result = a ^ b; break;
      case Kind::Equal: result = a == b ? 1u : 0u; break;
      case Kind::Less: result = signed32(a) < signed32(b) ? 1u : 0u; break;
      case Kind::LessEqual: result = signed32(a) <= signed32(b) ? 1u : 0u; break;
      case Kind::Greater: result = signed32(a) > signed32(b) ? 1u : 0u; break;
      case Kind::GreaterEqual: result = signed32(a) >= signed32(b) ? 1u : 0u; break;
      case Kind::HostCall:
        require(std::find(host.allowed.begin(), host.allowed.end(), n.immediate) != host.allowed.end(),
                "typed SSA host capability denied");
        require(static_cast<bool>(host.invoke), "typed SSA missing host callback");
        require(quota > 0, "typed SSA host budget exhausted");
        --quota;
        result = host.invoke(n.immediate, a);
        break;
      default: throw std::runtime_error("typed SSA invalid opcode");
    }
    values[i + 1] = result;
  }
  return values[m.result];
}

v4m3::Program compile_source_typed(const std::string& source) {
  return lower(parse_typed(source));
}
} // namespace venom::v4m8
