#include "compiler/pipeline/html.hpp"
#include "vm/opcode.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Node { std::string tag; int parent = -1; };

std::vector<Node> hydrate(const std::string& html) {
  venom::compiler::SiteGraph graph;
  graph.root = ".";
  venom::compiler::SiteFile file;
  file.relative = "index.html";
  file.extension = ".html";
  file.bytes.assign(html.begin(), html.end());
  graph.files.push_back(std::move(file));
  graph.routes.push_back("/");
  auto plan = venom::vm::make_polymorphic_plan(0x48594452u, false);
  auto compiled = venom::compiler::compile_html_routes(graph, plan);
  if (compiled.routes.size() != 1) throw std::runtime_error("expected one route");

  std::vector<Node> nodes{{"#root", -1}};
  std::vector<std::size_t> stack{0};
  std::size_t pending = nodes.size();
  for (const auto& ins : compiled.routes[0].program) {
    using venom::vm::LogicalOpcode;
    switch (ins.opcode) {
      case LogicalOpcode::CreateElement:
        if (ins.a >= compiled.strings.size()) throw std::runtime_error("bad string id");
        nodes.push_back({compiled.strings[ins.a], -1});
        pending = nodes.size() - 1;
        break;
      case LogicalOpcode::EnterElement:
        if (pending == nodes.size()) throw std::runtime_error("enter without element");
        stack.push_back(pending);
        pending = nodes.size();
        break;
      case LogicalOpcode::LeaveElement:
        if (stack.size() <= 1) throw std::runtime_error("route stack underflow");
        pending = stack.back();
        stack.pop_back();
        break;
      case LogicalOpcode::AppendChild:
        if (pending == nodes.size() || stack.empty()) throw std::runtime_error("append without child");
        nodes[pending].parent = static_cast<int>(stack.back());
        pending = nodes.size();
        break;
      default:
        break;
    }
  }
  if (stack.size() != 1) throw std::runtime_error("route stack did not return to root");
  return nodes;
}

std::size_t find_nth(const std::vector<Node>& nodes, const std::string& tag, int nth) {
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].tag == tag && --nth == 0) return i;
  }
  throw std::runtime_error("missing node: " + tag);
}

void require_siblings(const std::string& html, const std::string& tag, int count) {
  auto nodes = hydrate(html);
  int parent = -2;
  for (int n = 1; n <= count; ++n) {
    const std::size_t id = find_nth(nodes, tag, n);
    if (n == 1) parent = nodes[id].parent;
    if (nodes[id].parent != parent) throw std::runtime_error(tag + " elements were not siblings");
  }
}
}

int main() {
  try {
    require_siblings("<ul><li>one<li>two</ul>", "li", 2);
    require_siblings("<dl><dt>a<dd>b<dt>c</dl>", "dt", 2);
    require_siblings("<select><option>a<option>b</select>", "option", 2);
    require_siblings("<table><tr><td>a<td>b</table>", "td", 2);
    require_siblings("<p>text<div>block</div>", "p", 1);
    (void)hydrate("</div><section><b>x</section></orphan>");
    std::cout << "html route hydration regression: PASS\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "html route hydration regression: FAIL: " << e.what() << "\n";
    return 1;
  }
}
