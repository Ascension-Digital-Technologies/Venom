from pathlib import Path
import json

root = Path(__file__).resolve().parents[2]
plugin = (root / 'packages/vite/src/index.js').read_text(encoding='utf-8')
example = root / 'examples/react-vite-showcase'
pkg = json.loads((example / 'package.json').read_text(encoding='utf-8'))
config = (example / 'vite.config.js').read_text(encoding='utf-8')
source = (example / 'src/main.tsx').read_text(encoding='utf-8')

required_plugin_tokens = [
    'detectFramework', 'deps.react', 'deps["react-dom"]',
    'viteOutput', 'vite-write-bundle', 'mode === "vite-output"',
    "React detected; production protection will ingest Vite's compiled output"
]
for token in required_plugin_tokens:
    if token not in plugin:
        raise SystemExit(f'React/Vite integration is missing token: {token}')
if 'react' not in pkg['dependencies'] or 'react-dom' not in pkg['dependencies']:
    raise SystemExit('React showcase dependencies are incomplete')
if '@vitejs/plugin-react' not in config or '@venom/vite' not in config:
    raise SystemExit('React showcase does not combine the React and Venom Vite plugins')
if 'createRoot' not in source or 'useState' not in source:
    raise SystemExit('React showcase does not exercise a real React root and state update')
print('React Vite support smoke: PASS')

vite_config = (example / 'vite.config.js').read_text(encoding='utf-8')
assert 'preserveSymlinks: true' in vite_config
