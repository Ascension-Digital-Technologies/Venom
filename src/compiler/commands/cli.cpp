#include "compiler/commands/cli.hpp"
#include "compiler/core/config.hpp"
#include "compiler/core/version.hpp"

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

void apply_profile_defaults(BuildOptions& build) {
  if (build.profile != "dev" && build.profile != "prod") {
    throw std::runtime_error("--profile must be dev or prod");
  }

  // Both profiles execute protected exports through the real QuickJS/WASM
  // runtime. dev keeps readable generated JavaScript and diagnostics; prod
  // enables the complete hardened, stripped, fail-closed distribution.
  build.package_mode = "external";
  build.runtime = "wasm";
  build.quickjs_backend = "wasm-real";
  build.crypto_provider = "runtime";
  build.allow_host_fallback = false;
  build.deny_host_fallback = true;
  build.strict_release = true;
  build.security_target = "browser";
  build.hashed_assets = build.profile == "prod";
  build.emit_debug_manifest = build.profile == "dev";
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
  if (first == "dev") {
    cmd.kind = CommandKind::Dev;
    cmd.dev.executable = argv[0];
    cmd.dev.input = argc >= 3 && !is_flag(argv[2]) ? argv[2] : std::filesystem::current_path();
    for (int i = (argc >= 3 && !is_flag(argv[2])) ? 3 : 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--out") cmd.dev.output = require_value(i, argc, argv, arg);
      else if (arg == "--host") cmd.dev.host = require_value(i, argc, argv, arg);
      else if (arg == "--port") { const auto v=require_value(i,argc,argv,arg); const auto n=std::stoul(v); if(n<1||n>65535) throw std::runtime_error("--port must be 1..65535"); cmd.dev.port=static_cast<std::uint16_t>(n); }
      else if (arg == "--open") cmd.dev.open_browser = true;
      else if (arg == "--no-watch") cmd.dev.watch = false;
      else throw std::runtime_error("unknown dev option: " + arg);
    }
    return cmd;
  }

  if (first == "new" || first == "init") {
    cmd.kind = first == "new" ? CommandKind::NewProject : CommandKind::InitProject;
    int i = 2;
    if (first == "new") {
      if (argc < 3 || is_flag(argv[2])) throw std::runtime_error("usage: venom new <name-or-path> [--force]");
      cmd.project.directory = argv[2];
      i = 3;
    } else {
      cmd.project.directory = (argc >= 3 && !is_flag(argv[2])) ? argv[2] : std::filesystem::current_path();
      i = (argc >= 3 && !is_flag(argv[2])) ? 3 : 2;
    }
    for (; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--force") cmd.project.force = true;
      else throw std::runtime_error("unknown " + first + " option: " + arg);
    }
    return cmd;
  }
  if (first == "runtime") {
    cmd.kind = CommandKind::Runtime;
    if (argc < 3) throw std::runtime_error("usage: venom runtime install|verify|update|path [--dir <path>] [--force]");
    cmd.runtime.action = argv[2];
    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--dir") cmd.runtime.directory = require_value(i, argc, argv, arg);
      else if (arg == "--force") cmd.runtime.force = true;
      else throw std::runtime_error("unknown runtime option: " + arg);
    }
    return cmd;
  }
  if (first == "config") {
    cmd.kind = CommandKind::Config;
    if (argc < 3) throw std::runtime_error("usage: venom config validate|print [path] [--format text|json]");
    cmd.config.action = argv[2];
    int i = 3;
    if (i < argc && !is_flag(argv[i])) cmd.config.file = argv[i++];
    for (; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--format") {
        const auto value = require_value(i, argc, argv, arg);
        if (value == "json") cmd.config.format = OutputFormat::Json;
        else if (value != "text") throw std::runtime_error("--format must be text or json");
      } else throw std::runtime_error("unknown config option: " + arg);
    }
    return cmd;
  }

  if (first == "update") {
    cmd.kind = CommandKind::Update;
    cmd.update.action = "check";
    int i = 2;
    if (i < argc && !is_flag(argv[i])) cmd.update.action = argv[i++];
    if (cmd.update.action != "check" && cmd.update.action != "install" && cmd.update.action != "rollback" && cmd.update.action != "status") {
      throw std::runtime_error("update action must be check, install, rollback, or status");
    }
    for (; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--channel") { const auto v=require_value(i,argc,argv,arg); if(v!="stable"&&v!="preview") throw std::runtime_error("--channel must be stable or preview"); cmd.update.channel=v; }
      else if (arg == "--manifest") cmd.update.manifest=require_value(i,argc,argv,arg);
      else if (arg == "--prefix") cmd.update.prefix=require_value(i,argc,argv,arg);
      else if (arg == "--public-key") cmd.update.public_key=require_value(i,argc,argv,arg);
      else if (arg == "--require-signature") cmd.update.require_signature=true;
      else if (arg == "--yes") cmd.update.yes=true;
      else if (arg == "--dry-run") cmd.update.dry_run=true;
      else if (arg == "--format") { const auto v=require_value(i,argc,argv,arg); if(v=="json") cmd.update.format=OutputFormat::Json; else if(v!="text") throw std::runtime_error("--format must be text or json"); }
      else throw std::runtime_error("unknown update option: "+arg);
    }
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
      } else if (arg == "--profile") {
        const auto value = require_value(i, argc, argv, arg);
        if (value != "development" && value != "production" && value != "runtime-contributor")
          throw std::runtime_error("--profile must be development, production, or runtime-contributor");
        cmd.doctor.profile = value;
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

  if (first == "analyze-dist") {
    cmd.kind = CommandKind::AnalyzeDist;
    if (argc < 3) throw std::runtime_error("usage: venom analyze-dist <dist-dir> [--format text|json]");
    cmd.analyze_dist_input = argv[2];
    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--format") {
        const auto value = require_value(i, argc, argv, arg);
        if (value == "json") cmd.analyze_dist_format = OutputFormat::Json;
        else if (value != "text") throw std::runtime_error("--format must be text or json");
      } else throw std::runtime_error("unknown analyze-dist option: " + arg);
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

  const auto explicit_input = cmd.build.input;
  std::filesystem::path config_path;
  for (int i = option_start; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config") config_path = require_value(i, argc, argv, arg);
  }
  if (config_path.empty()) config_path = discover_project_config(cmd.build.input.empty() ? std::filesystem::current_path() : cmd.build.input);
  if (!config_path.empty()) apply_project_config(config_path, cmd.build);
  // An explicit positional site directory always wins over project.entry.
  // This keeps `venom build examples/foo` deterministic even when that project
  // has a local venom.toml containing entry = ".".
  if (!explicit_input.empty()) cmd.build.input = explicit_input;
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
    } else if (arg == "--seed") {
      const auto value = require_value(i, argc, argv, arg);
      std::size_t consumed = 0;
      const auto parsed = std::stoull(value, &consumed, 0);
      if (consumed != value.size() || parsed == 0 || parsed > 0xffffffffull) {
        throw std::runtime_error("--seed must be an integer from 1 to 4294967295");
      }
      cmd.build.diversification_seed = static_cast<std::uint32_t>(parsed);
    } else {
      throw std::runtime_error("unknown build option: " + arg);
    }
  }

  apply_profile_defaults(cmd.build);

  if (cmd.build.allow_host_fallback && cmd.build.deny_host_fallback) {
    throw std::runtime_error("--allow-host-fallback and --deny-host-fallback cannot be used together");
  }
  if (cmd.build.strict_release && cmd.build.allow_host_fallback) {
    throw std::runtime_error("--strict-release cannot be combined with --allow-host-fallback");
  }
  const bool protected_profile = true;
  if (protected_profile && cmd.build.allow_host_fallback) {
    throw std::runtime_error("host fallback is not available in dev or prod builds");
  }
  if (cmd.build.quickjs_backend != "wasm-real") {
    throw std::runtime_error("dev and prod require --quickjs-backend wasm-real");
  }
  if (cmd.build.crypto_provider != "runtime") {
    throw std::runtime_error("dev and prod browser builds require --crypto-provider runtime");
  }

  if (cmd.build.require_audited_crypto && cmd.build.crypto_provider != "libsodium") {
    throw std::runtime_error("--require-audited-crypto is not available for browser dev/prod builds");
  }

  return cmd;
}

void print_help() {
  std::cout << VENOM_PRODUCT_NAME << " " << VENOM_VERSION_STRING << "\n\n"
            << "Usage:\n"
            << "  venom build [site-dir] --profile dev|prod [--out <dist>] [--seed <u32>]\n"
            << "  venom dev [site-dir] [--out dist-dev] [--port 8080] [--open]\n"
            << "  venom new <name-or-path> [--force]\n"
            << "  venom init [directory] [--force]\n"
            << "  venom runtime install|verify|update|path [--dir <path>]\n"
            << "  venom update [check|install|rollback|status] [--channel stable|preview]\n"
            << "  venom config validate|print [venom.toml] [--format text|json]\n"
            << "  venom doctor [--profile development|production|runtime-contributor] [--format text|json]\n"
            << "  venom analyze <site-dir> [--format text|json]\n"
            << "  venom analyze-dist <dist-dir> [--format text|json]\n"
            << "  venom compatibility check <site-dir> [--format text|json]\n"
            << "  venom contracts [--format text|json]\n"
            << "  venom release-check <dist-or-package> [--target browser]\n"
            << "  venom verify-runtime <dist-or-package> [--require-real-engine]\n"
            << "  venom inspect <dist/assets/app/app.<hash>.vbc>\n"
            << "  venom --version\n\n"
            << "Build profiles:\n"
            << "  dev   Readable generated runtime, diagnostics, stable asset names, real QuickJS/WASM protection.\n"
            << "  prod  Obfuscated, hashed, stripped, diversified, fail-closed deployment output.\n\n"
            << "Project setup:\n"
            << "  venom new my-site\n"
            << "  cd my-site && venom dev . --open\n"
            << "  venom runtime install\n"
            << "  venom config validate\n\n"
            << "Examples:\n"
            << "  venom dev examples/protected-chess --open\n"
            << "  venom build examples/protected-chess --out dist-dev --profile dev\n"
            << "  venom build examples/protected-chess --out dist --profile prod\n"
            << "  venom build examples/protected-chess --out dist --profile prod --seed 123456\n"
            << "  scripts\\build-site.bat examples\\protected-chess dist prod\n"
            << "  scripts\\serve-site.bat 8080 dist\n";
}

void print_version() {
  std::cout << "venom " << VENOM_VERSION_STRING << '\n';
}

} // namespace venom::compiler
