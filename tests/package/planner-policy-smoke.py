from pathlib import Path
root=Path(__file__).resolve().parents[2]
cli=(root/'src/cli/cli.cpp').read_text()
header=(root/'src/cli/cli.hpp').read_text()
planner=(root/'src/pipeline/planner.cpp').read_text()
build=(root/'src/pipeline/build.cpp').read_text()
config=(root/'src/core/config.cpp').read_text()
doc=(root/'docs/architecture/product-overview.md').read_text()
for token in ['--protect','--browser','--planner','--protection','--min-confidence','--planner-min-confidence','--report']:
    assert token in cli, token
for token in ['FunctionRecommendation','function purity score','manual-review','schema_version\\\":2','strict planner rejected']:
    assert token in planner, token
assert 'enforce_build_protection_plan(options)' in build
assert 'planner.protect' in config and 'planner.browser' in config
assert 'annotations > CLI overrides > config rules > planner > protected default' in planner
assert 'A higher-precedence decision cannot be overwritten by the planner' in doc
assert 'server_required\\\":false' in planner
assert 'planner_minimum_confidence' in header
print('planner policy smoke: PASS')
