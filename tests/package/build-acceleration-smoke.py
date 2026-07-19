#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[2]
fail=[]
cm=(root/'cmake/build_acceleration.cmake').read_text()
main=(root/'CMakeLists.txt').read_text()
domains=(root/'cmake/source_domains.cmake').read_text()
pch=(root/'include/venom/internal/core/pch.hpp')
tool=(root/'tools/build_performance.py')
for token in ('VENOM_COMPILER_CACHE','VENOM_ENABLE_PCH','VENOM_ENABLE_MSVC_MP','VENOM_ENABLE_QUICKJS_IPO','CMAKE_CXX_COMPILER_LAUNCHER'):
    if token not in cm: fail.append(f'missing build acceleration contract: {token}')
for token in ('venom_configure_compiler_cache()','venom_apply_build_acceleration(qjs)'):
    if token not in main: fail.append(f'CMake missing acceleration integration: {token}')
for token in ('venom_apply_build_acceleration(${target})','target_precompile_headers(${target}'):
    if token not in domains: fail.append(f'domain CMake missing acceleration integration: {token}')
if not pch.exists(): fail.append('missing compiler PCH')
if not tool.exists(): fail.append('missing build performance tool')
else:
    text=tool.read_text()
    for token in ('clean_build','noop_build','incremental_build','venom.build-performance.v1'):
        if token not in text: fail.append(f'performance tool missing {token}')
if fail:
    print('\n'.join('failure: '+x for x in fail)); raise SystemExit(1)
print('build acceleration smoke: PASS')
