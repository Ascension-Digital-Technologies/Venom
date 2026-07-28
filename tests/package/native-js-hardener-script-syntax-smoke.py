from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
source = (root / "src/pipeline/native_js_hardener.cpp").read_text(encoding="utf-8")
match = re.search(r'const char kRunHardener\[\] = R"JS\((.*?)\)JS";', source, re.S)
assert match, "embedded kRunHardener script not found"
script = match.group(1)

# Regression for the release-blocking parse failure where an extra brace closed
# the outer try before its catch/finally clause.
assert "\n    }\n    }\n    const bindingMarker" not in script
assert "\n    }\n    const bindingMarker" in script
print("native JS hardener embedded-script syntax source checks passed")
