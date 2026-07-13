#include "compiler/cli.hpp"
#include "compiler/config.hpp"
#include "compiler/version.hpp"

#include <iostream>
#include <stdexcept>

namespace venom::compiler {

namespace {
bool is_flag(const std::string& value) {
  return value.rfind("--", 0) == 0;
}

std::string require_value(int& i, int argc, char** argv, const std::string& flag) {
  if (i + 1 >= argc || is_flag(argv[i + 1])) {
    throw std::runtime_error("missing value for " + flag);
  }
  ++i;
  return argv[i];
}

void force_production_build_defaults(BuildOptions& build) {
  // Venom is intentionally production-only now. Historical debug/protect/compat
  // flags are accepted by older scripts, but the emitted artifact is always the
  // hardened browser target:
  //   index.html
  //   assets/app/app.<hash>.vbc
  //   assets/style/s.<hash>.css
  //   assets/images/<name>.<hash>.<ext>
  //   assets/loader/loader.<hash>.js
  //   assets/runtime/{r,engine,runtime,rw}.<hash>.*
  //   assets/workers/worker.<hash>.js
  build.profile = "browser-protect";
  build.hashed_assets = true;
  build.package_mode = "external";
  build.runtime = "wasm";
  build.quickjs_backend = "wasm-real";
  build.crypto_provider = "runtime";
  build.allow_host_fallback = false;
  build.deny_host_fallback = true;
  build.strict_release = true;
  build.emit_debug_manifest = false;
  build.security_target = "browser";
}

} // namespace

Command parse_command(int argc, char** argv) {
  Command cmd;
  if (argc <= 1) {
    return cmd;
  }

  const std::string first = argv[1];
  if (first == "--help" || first == "-h" || first == "help") {
    cmd.kind = CommandKind::Help;
    return cmd;
  }
  if (first == "--version" || first == "version") {
    cmd.kind = CommandKind::Version;
    return cmd;
  }
  if (first == "doctor") {
    cmd.kind = CommandKind::Doctor;
    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--format") {
        const auto value = require_value(i, argc, argv, arg);
        if (value == "json") cmd.doctor.format = OutputFormat::Json;
        else if (value != "text") throw std::runtime_error("--format must be text or json");
      } else {
        throw std::runtime_error("unknown doctor option: " + arg);
      }
    }
    return cmd;
  }


  if (first == "analyze") {
    cmd.kind = CommandKind::Analyze;
    if (argc < 3) throw std::runtime_error("usage: venom analyze <site-dir> [--format text|json]");
    cmd.compatibility.input = argv[2];
    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--format") {
        const auto value = require_value(i, argc, argv, arg);
        if (value == "json") cmd.compatibility.format = OutputFormat::Json;
        else if (value != "text") throw std::runtime_error("--format must be text or json");
      } else throw std::runtime_error("unknown analyze option: " + arg);
    }
    return cmd;
  }

  if (first == "compatibility") {
    cmd.kind = CommandKind::Compatibility;
    if (argc < 4 || std::string(argv[2]) != "check") throw std::runtime_error("usage: venom compatibility check <site-dir> [--format text|json]");
    cmd.compatibility.input = argv[3];
    for (int i = 4; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--format") {
        const auto value = require_value(i, argc, argv, arg);
        if (value == "json") cmd.compatibility.format = OutputFormat::Json;
        else if (value != "text") throw std::runtime_error("--format must be text or json");
      } else throw std::runtime_error("unknown compatibility option: " + arg);
    }
    return cmd;
  }
  if (first == "contracts") {
    cmd.kind = CommandKind::Contracts;
    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--format") {
        const auto value = require_value(i, argc, argv, arg);
        if (value == "json") cmd.contracts.format = OutputFormat::Json;
        else if (value != "text") throw std::runtime_error("--format must be text or json");
      } else throw std::runtime_error("unknown contracts option: " + arg);
    }
    return cmd;
  }

  if (first == "inspect") {
    cmd.kind = CommandKind::Inspect;
    if (argc < 3) {
      throw std::runtime_error("inspect requires a package path");
    }
    cmd.inspect.package = argv[2];
    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--key-file") {
        cmd.inspect.key_file = require_value(i, argc, argv, arg);
      } else {
        throw std::runtime_error("unknown inspect option: " + arg);
      }
    }
    return cmd;
  }

  if (first == "keygen") {
    cmd.kind = CommandKind::Keygen;
    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--out") {
        cmd.keygen.output = require_value(i, argc, argv, arg);
      } else if (arg == "--force") {
        cmd.keygen.force = true;
      } else {
        throw std::runtime_error("unknown keygen option: " + arg);
      }
    }
    return cmd;
  }

  if (first == "release-check" || first == "verify-runtime") {
    cmd.kind = first == "verify-runtime" ? CommandKind::VerifyRuntime : CommandKind::ReleaseCheck;
    cmd.release_check.runtime_verification = first == "verify-runtime";
    if (argc < 3) {
      throw std::runtime_error(first + " requires a dist directory or package path");
    }
    cmd.release_check.target = argv[2];
    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--key-file") {
        cmd.release_check.key_file = require_value(i, argc, argv, arg);
      } else if (arg == "--target") {
        cmd.release_check.security_target = require_value(i, argc, argv, arg);
        if (cmd.release_check.security_target != "auto" && cmd.release_check.security_target != "browser" && cmd.release_check.security_target != "native") {
          throw std::runtime_error("--target must be auto, browser, or native");
        }
      } else if (arg == "--require-audited-crypto") {
        cmd.release_check.require_audited_crypto = true;
      } else if (arg == "--require-real-engine") {
        cmd.release_check.require_real_engine = true;
      } else {
        throw std::runtime_error("unknown " + first + " option: " + arg);
      }
    }
    return cmd;
  }

  if (first != "build") {
    throw std::runtime_error("unknown command: " + first);
  }

  cmd.kind = CommandKind::Build;
  int option_start = 2;
  if (argc >= 3 && !is_flag(argv[2])) {
    cmd.build.input = argv[2];
    option_start = 3;
  }

  std::filesystem::path config_path;
  for (int i = option_start; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config") config_path = require_value(i, argc, argv, arg);
  }
  if (config_path.empty()) config_path = discover_project_config(cmd.build.input.empty() ? std::filesystem::current_path() : cmd.build.input);
  if (!config_path.empty()) apply_project_config(config_path, cmd.build);
  if (cmd.build.input.empty()) throw std::runtime_error("build requires an input directory or project.entry in venom.toml");

  for (int i = option_start; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config") {
      (void)require_value(i, argc, argv, arg);
    } else if (arg == "--format") {
      const auto value = require_value(i, argc, argv, arg);
      if (value == "json") cmd.build.format = OutputFormat::Json;
      else if (value != "text") throw std::runtime_error("--format must be text or json");
    } else if (arg == "--out") {
      cmd.build.output = require_value(i, argc, argv, arg);
    } else if (arg == "--profile") {
      cmd.build.profile = require_value(i, argc, argv, arg);
    } else if (arg == "--hashed") {
      cmd.build.hashed_assets = true;
    } else if (arg == "--package-mode") {
      cmd.build.package_mode = require_value(i, argc, argv, arg);
    } else if (arg == "--runtime") {
      cmd.build.runtime = require_value(i, argc, argv, arg);
      if (cmd.build.runtime != "js" && cmd.build.runtime != "wasm") throw std::runtime_error("--runtime must be js or wasm");
    } else if (arg == "--quickjs-backend") {
      cmd.build.quickjs_backend = require_value(i, argc, argv, arg);
    } else if (arg == "--crypto-provider") {
      cmd.build.crypto_provider = require_value(i, argc, argv, arg);
    } else if (arg == "--allow-host-fallback" || arg == "--allow-release-host-fallback") {
      cmd.build.allow_host_fallback = true;
    } else if (arg == "--deny-host-fallback") {
      cmd.build.deny_host_fallback = true;
    } else if (arg == "--strict-release") {
      cmd.build.strict_release = true;
    } else if (arg == "--emit-debug-manifest") {
      cmd.build.emit_debug_manifest = true;
    } else if (arg == "--key-file") {
      cmd.build.key_file = require_value(i, argc, argv, arg);
    } else if (arg == "--require-audited-crypto") {
      cmd.build.require_audited_crypto = true;
    } else if (arg == "--target") {
      cmd.build.security_target = require_value(i, argc, argv, arg);
    } else if (arg == "--vendor-cache") {
      cmd.build.vendor_cache = require_value(i, argc, argv, arg);
    } else if (arg == "--vendor-lock") {
      cmd.build.vendor_lock = require_value(i, argc, argv, arg);
    } else if (arg == "--offline") {
      cmd.build.vendor_offline = true;
    } else if (arg == "--refresh-vendors") {
      cmd.build.refresh_vendors = true;
    } else {
      throw std::runtime_error("unknown build option: " + arg);
    }
  }

  force_production_build_defaults(cmd.build);

  if (cmd.build.allow_host_fallback && cmd.build.deny_host_fallback) {
    throw std::runtime_error("--allow-host-fallback and --deny-host-fallback cannot be used together");
  }
  if (cmd.build.strict_release && cmd.build.allow_host_fallback) {
    throw std::runtime_error("--strict-release cannot be combined with --allow-host-fallback");
  }
  const bool protected_profile = cmd.build.profile == "protect" || cmd.build.profile == "browser-protect" ||
                                 cmd.build.profile == "native-protect" || cmd.build.profile == "release";
  if (protected_profile && cmd.build.allow_host_fallback) {
    throw std::runtime_error("--allow-host-fallback is only valid with --profile debug; protected and release profiles are structurally fail-closed");
  }
  if (cmd.build.profile == "browser-protect") {
    if (cmd.build.crypto_provider != "runtime") {
      throw std::runtime_error("--profile browser-protect requires --crypto-provider runtime");
    }
    if (cmd.build.quickjs_backend == "scaffold") {
      cmd.build.quickjs_backend = "wasm-real";
    }
    if (cmd.build.quickjs_backend != "wasm-real") {
      throw std::runtime_error("--profile browser-protect requires --quickjs-backend wasm-real");
    }
    cmd.build.deny_host_fallback = true;
    if (cmd.build.security_target == "auto") {
      cmd.build.security_target = "browser";
    }
  }
  if (cmd.build.profile == "native-protect") {
    if (cmd.build.quickjs_backend == "scaffold") {
      cmd.build.quickjs_backend = "wasm-real";
    }
    if (cmd.build.crypto_provider == "runtime") {
      cmd.build.crypto_provider = "libsodium";
    }
    if (cmd.build.crypto_provider != "libsodium") {
      throw std::runtime_error("--profile native-protect requires --crypto-provider libsodium");
    }
    if (cmd.build.security_target == "auto") {
      cmd.build.security_target = "native";
    }
  }
  if (cmd.build.require_audited_crypto && cmd.build.crypto_provider != "libsodium") {
    throw std::runtime_error("--require-audited-crypto requires --crypto-provider libsodium or --profile native-protect");
  }

  return cmd;
}

void print_help() {
  std::cout << VENOM_PRODUCT_NAME << " " << VENOM_VERSION_STRING << "\n\n"
            << "Usage:\n"
            << "  venom build [site-dir] [--config venom.toml] [--out <dist>] [--format text|json]\n"
            << "  venom doctor [--format text|json]\n"
            << "  venom analyze <site-dir> [--format text|json]\n"
            << "  venom compatibility check <site-dir> [--format text|json]\n"
            << "  venom contracts [--format text|json]\n"
            << "  venom keygen --out venom.key [--force]\n"
            << "  venom release-check <dist-or-package> [--target browser] [--key-file venom.key]\n"
            << "  venom verify-runtime <dist-or-package> [--target browser] [--key-file venom.key] [--require-real-engine]\n"
            << "  venom inspect <dist/assets/app/app.<hash>.vbc> [--key-file venom.key]\n"
            << "  venom --version\n\n"
            << "Production build policy:\n"
            << "  build always emits the hardened browser target, vendors HTTPS scripts into protected bytecode,\n"
            << "  and pins every remote dependency in source-local venom.lock.\n"
            << "  --refresh-vendors is the only operation that updates reviewed dependency pins.\n\n"
            << "Examples:\n"
            << "  venom build examples/basic-site --out dist\n"
            << "  scripts\\build-site.bat examples\\basic-site dist\n"
            << "  scripts\\serve-site.bat 8080 dist\n";
}

void print_version() {
  std::cout << "venom " << VENOM_VERSION_STRING << '\n';
}

} // namespace venom::compiler
