from pathlib import Path
import sys
root=Path(__file__).resolve().parents[2]
checks={
  'decompiler directory': (root/'src/decompiler/decompiler.cpp').is_file(),
  'decompiler executable target': 'add_executable(venom-decompiler' in (root/'CMakeLists.txt').read_text(errors='ignore'),
  'protected chess zero-bytecode gate': 'quickjs_bytecode_records' in (root/'tests/package/protected-chess-engine-smoke.py').read_text(errors='ignore'),
  'release leak scan': any('leak' in p.name for p in (root/'tests/package').glob('*leak*')),
  'native bytecode security test': (root/'tests/quickjs/native-bytecode-security.cpp').is_file(),
}
failed=[k for k,v in checks.items() if not v]
for k,v in checks.items(): print(f"[{'PASS' if v else 'FAIL'}] {k}")
if failed: raise SystemExit('assurance checks failed: '+', '.join(failed))
print('protected runtime assurance smoke passed')
