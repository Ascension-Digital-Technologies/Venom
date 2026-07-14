#include "decompiler/decompiler.hpp"
#include "package/hash.hpp"
#include "quickjs/bytecode.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void append_u32(std::vector<unsigned char>& out, std::uint32_t value) {
  out.push_back(static_cast<unsigned char>(value & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
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
  std::uint32_t hash = 2166136261u;
  for (const auto lane : map) { hash ^= lane; hash *= 16777619u; }
  return hash;
}

std::vector<unsigned char> wrap_record(const std::vector<unsigned char>& raw, std::uint32_t seed) {
  const auto map = lane_map(seed);
  std::vector<unsigned char> out;
  out.insert(out.end(), {'V','Q','J','S','E','0','0','6'});
  append_u32(out, 2u);
  append_u32(out, static_cast<std::uint32_t>(raw.size()));
  append_u32(out, seed);
  append_u32(out, 0x01000300u);
  const auto hash = venom::package::fnv1a64(raw);
  for (int i = 0; i < 8; ++i) out.push_back(static_cast<unsigned char>((hash >> (i * 8)) & 0xffu));
  append_u32(out, 48u);
  append_u32(out, 0u);
  append_u32(out, 16u);
  append_u32(out, lane_fingerprint(map));
  std::uint32_t stream = seed ^ 0x9E3779B9u;
  for (std::size_t block = 0; block < raw.size(); block += 16u) {
    const std::size_t block_size = std::min<std::size_t>(16u, raw.size() - block);
    std::array<unsigned char, 16> block_map{};
    std::size_t block_map_size = 0;
    for (const auto lane : map) if (lane < block_size) block_map[block_map_size++] = lane;
    for (std::size_t stored_lane = 0; stored_lane < block_size; ++stored_lane) {
      if (((block + stored_lane) & 3u) == 0u) xorshift32(stream);
      const auto mask = static_cast<unsigned char>((stream >> (((block + stored_lane) & 3u) * 8u)) & 0xffu);
      out.push_back(static_cast<unsigned char>(raw[block + block_map[stored_lane]] ^ mask));
    }
  }
  return out;
}

void write_file(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!out) throw std::runtime_error("failed to write test artifact");
}

std::string read_text(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void require(bool value, const std::string& message) {
  if (!value) throw std::runtime_error(message);
}

} // namespace

int main() {
  try {
    const auto root = std::filesystem::temp_directory_path() / "venom-decompiler-quickjs-smoke";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const std::string source = "function score(x){const salt='visible-constant';return Math.imul(x+7,2654435761)>>>0;} score(9);";
    const auto record = venom::quickjs::compile_native_quickjs_bytecode(source, "protected-score.js", false, true);
    const auto envelope = wrap_record(record, 0x51ED270Bu);
    const auto input = root / "protected-score.vqjs";
    const auto output = root / "recovered-record";
    write_file(input, envelope);
    require(venom::decompiler::recover({input, output, {}, venom::compiler::OutputFormat::Text, false, true, true}),
            "record recovery returned false");
    const auto artifact = output / "quickjs-artifact" / input.filename();
    require(std::filesystem::exists(artifact / "decoded-record.bin"), "decoded record missing");
    require(std::filesystem::exists(artifact / "quickjs-object.bin"), "QuickJS object missing");
    require(std::filesystem::file_size(artifact / "quickjs-object.bin") > 0u, "QuickJS object is empty");
    require(read_text(artifact / "record.json").find("\"sourcePresent\": false") != std::string::npos,
            "source stripping metadata missing");
    require(std::filesystem::exists(artifact / "static-analysis.json"), "static analysis metadata missing");
    require(std::filesystem::exists(artifact / "probable-identifiers.txt"), "identifier inventory missing");
    require(std::filesystem::exists(artifact / "urls.txt"), "URL inventory missing");
    require(std::filesystem::exists(artifact / "module-paths.txt"), "module-path inventory missing");
    require(read_text(artifact / "static-analysis.json").find("\"entropyBitsPerByte\"") != std::string::npos,
            "bytecode entropy was not reported");
    require(read_text(artifact / "printable-strings.txt").find("visible-constant") != std::string::npos,
            "recoverable QuickJS constant was not inventoried");
    require(read_text(output / "recovery-report.json").find("\"quickJsRecords\": 1") != std::string::npos,
            "record count was not reported");
    require(read_text(output / "recovery-report.json").find("\"quickJsPrintableStrings\"") != std::string::npos,
            "QuickJS string analysis totals were not reported");

    auto corrupted = envelope;
    corrupted.back() ^= 0x5Au;
    const auto corrupted_input = root / "corrupted.vqjs";
    write_file(corrupted_input, corrupted);
    bool rejected_corruption = false;
    try {
      (void)venom::decompiler::recover({corrupted_input, root / "corrupted-output", {}, venom::compiler::OutputFormat::Text, false, false, true});
    } catch (const std::exception&) {
      rejected_corruption = true;
    }
    require(rejected_corruption, "corrupted QuickJS envelope was accepted");

    std::vector<venom::quickjs::ModuleSourceRecord> modules{
      {"entry.js", "entry.js", "export function answer(){return 42;}"},
      {"helper.js", "helper.js", "export const label='module-constant';"}
    };
    const auto bundle = venom::quickjs::compile_native_quickjs_module_bundle("entry.js", modules, true);
    const auto bundle_input = root / "module-bundle.vqjs";
    const auto bundle_output = root / "recovered-bundle";
    write_file(bundle_input, wrap_record(bundle, 0xA5C31F27u));
    require(venom::decompiler::recover({bundle_input, bundle_output, {}, venom::compiler::OutputFormat::Text, false, true, true}),
            "module bundle recovery returned false");
    const auto bundle_dir = bundle_output / "quickjs-artifact" / bundle_input.filename();
    require(std::filesystem::exists(bundle_dir / "module-bundle.json"), "module bundle metadata missing");
    require(std::filesystem::exists(bundle_dir / "module-0-entry.js" / "quickjs-object.bin"), "entry module object missing");
    require(std::filesystem::exists(bundle_dir / "module-1-helper.js" / "quickjs-object.bin"), "helper module object missing");
    require(read_text(bundle_output / "recovery-report.json").find("\"quickJsModules\": 2") != std::string::npos,
            "module count was not reported");

    std::filesystem::remove_all(root);
    std::cout << "venom decompiler QuickJS artifact smoke passed\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
