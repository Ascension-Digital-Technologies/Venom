from pathlib import Path
root = Path(__file__).resolve().parents[2]
js = (root / 'src/compiler/js.cpp').read_text(encoding='utf-8')
build = (root / 'src/compiler/build.cpp').read_text(encoding='utf-8')
required = [
    'VENOM_EXECUTION_PLAN_V1',
    'execution_plan_text',
    'execution_plan_json',
    'classic-script realm closure triggered by',
    'explicit browser directive',
    'explicit protected directive',
    'no browser-only evidence',
]
for marker in required:
    if marker not in js:
        raise SystemExit(f'missing execution planner marker: {marker}')
for marker in ['options.output / "build" / "reports"', 'execution-plan.txt', 'execution-plan.json']:
    if marker not in build:
        raise SystemExit(f'missing report emission marker: {marker}')
print('execution plan report smoke passed')
