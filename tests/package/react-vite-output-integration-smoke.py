from pathlib import Path
import subprocess
root = Path(__file__).resolve().parents[2]
result = subprocess.run(['node', str(root / 'tests/integration/vite-react-output-smoke.mjs')], cwd=root, text=True, capture_output=True)
if result.returncode:
    raise SystemExit(result.stdout + result.stderr)
print(result.stdout.strip())
