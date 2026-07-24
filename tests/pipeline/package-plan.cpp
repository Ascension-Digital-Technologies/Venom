#include "planning/package_plan.hpp"

#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

int main() {
  namespace fs = std::filesystem;
  using venom::compiler::package_plan::Plan;
  using venom::compiler::package_plan::WriterOptions;

  const auto output = fs::temp_directory_path() / "venom-package-plan-smoke.vbc";
  std::error_code ignored;
  fs::remove(output, ignored);

  Plan plan;
  plan.add(venom::package::SectionType::Manifest,
           "manifest.txt",
           std::vector<unsigned char>{'o', 'k'});
  plan.add(venom::package::SectionType::JavaScript,
           "entry.js",
           std::vector<unsigned char>{'e', 'x', 'p', 'o', 'r', 't', ' ', '{', '}', ';'});

  assert(plan.size() == 2u);
  WriterOptions options;
  options.package_flags = venom::package::PackageFlagNone;
  options.compression_enabled = true;
  options.layout_polymorphism_enabled = false;

  const auto artifact = plan.write_verified(output, options);
  assert(!artifact.bytes.empty());
  assert(artifact.package.sections.size() == 2u);
  assert(fs::exists(output));

  fs::remove(output, ignored);
  return 0;
}
