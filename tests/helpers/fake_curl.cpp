#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  std::filesystem::path output;
  std::filesystem::path error_output;
  std::string user_agent;
  std::string url;
  std::vector<std::string> unexpected_positionals;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--output" && i + 1 < argc) {
      output = argv[++i];
    } else if (arg == "--stderr" && i + 1 < argc) {
      error_output = argv[++i];
    } else if (arg == "--user-agent" && i + 1 < argc) {
      user_agent = argv[++i];
    } else if (arg == "--url" && i + 1 < argc) {
      url = argv[++i];
    } else if (!arg.empty() && arg[0] != '-') {
      // Values belonging to known options have already been consumed above or
      // are skipped here. Any second positional value is a process-quoting bug.
      const std::string previous = i > 1 ? argv[i - 1] : "";
      if (previous != "--connect-timeout" && previous != "--max-time" &&
          previous != "--max-redirs" && previous != "--max-filesize") {
        if (url.empty()) {
          url = arg;
        } else {
          unexpected_positionals.push_back(arg);
        }
      }
    }
  }

  const char* expected_user_agent = std::getenv("VENOM_FAKE_CURL_EXPECT_USER_AGENT");
  if (expected_user_agent != nullptr && user_agent != expected_user_agent) {
    if (!error_output.empty()) {
      std::ofstream error(error_output, std::ios::binary | std::ios::trunc);
      error << "fake curl user-agent boundary mismatch: [" << user_agent << "]\n";
    }
    return 2;
  }
  if (!unexpected_positionals.empty()) {
    if (!error_output.empty()) {
      std::ofstream error(error_output, std::ios::binary | std::ios::trunc);
      error << "fake curl received unexpected positional argument: "
            << unexpected_positionals.front() << "\n";
    }
    return 2;
  }
  if (output.empty() || url.empty()) {
    std::cerr << "fake curl requires --output and a URL\n";
    return 2;
  }
  if (url.find("download-failure") != std::string::npos) {
    if (!error_output.empty()) {
      std::ofstream error(error_output, std::ios::binary | std::ios::trunc);
      error << "simulated curl download failure\n";
    }
    return 22;
  }
  const char* no_output = std::getenv("VENOM_FAKE_CURL_NO_OUTPUT");
  if (no_output != nullptr && *no_output != '\0') {
    std::cout << "simulated response body that must not be mistaken for a file\n";
    return 0;
  }
  std::ofstream stream(output, std::ios::binary | std::ios::trunc);
  if (!stream) {
    return 23;
  }
  if (url.find("html-response") != std::string::npos) {
    stream << "<!doctype html><html><body>not javascript</body></html>";
  } else if (url.find("empty-response") != std::string::npos) {
    // Intentionally empty.
  } else {
    stream << "globalThis.__venomFetchedVendor=(globalThis.__venomFetchedVendor||0)+1;\n";
    const char* variant = std::getenv("VENOM_FAKE_CURL_VARIANT");
    if (variant != nullptr && *variant != '\0') {
      stream << "globalThis.__venomFetchedVendorVariant='" << variant << "';\n";
    }
  }
  return stream ? 0 : 23;
}
