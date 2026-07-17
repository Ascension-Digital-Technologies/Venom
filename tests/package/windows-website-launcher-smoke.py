from pathlib import Path

root = Path(__file__).resolve().parents[2]
script = root / "venom-api-server" / "scripts" / "windows" / "build-and-launch.ps1"
text = script.read_text(encoding="utf-8")
assert "$buildParams = @{ Target = 'api' }" in text
assert "@buildParams" in text
assert "$buildArgs = @('-Target', 'api')" not in text
print("windows website launcher smoke: PASS")
