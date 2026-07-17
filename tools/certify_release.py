#!/usr/bin/env python3
"""Create fail-closed, machine-readable Venom release certification evidence."""
from __future__ import annotations
import argparse, hashlib, json, os, platform, re, shutil, subprocess, sys, tempfile, time
from datetime import datetime, timezone
from pathlib import Path

SCHEMA = "VENOM_RELEASE_CERTIFICATION_RESULT_V1"

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()

def version_from_cmake(root: Path) -> str:
    text = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    match = re.search(r'\bVERSION\s+(\d+\.\d+\.\d+)', text)
    if not match:
        raise RuntimeError('project version is missing from CMakeLists.txt')
    return match.group(1)

def run_step(step_id: str, command: list[str], cwd: Path, env: dict[str, str]) -> dict:
    started = time.perf_counter()
    print(f'[certify] {step_id}: ' + ' '.join(command), flush=True)
    return_code = 124
    output = ''
    with tempfile.TemporaryFile(mode='w+b') as stream:
        try:
            proc = subprocess.run(command, cwd=cwd, env=env, stdout=stream, stderr=subprocess.STDOUT, timeout=120)
            return_code = proc.returncode
        except subprocess.TimeoutExpired:
            return_code = 124
        stream.seek(0)
        output = stream.read().decode('utf-8', errors='replace')
    if return_code == 124:
        output += '\ncertification step timed out after 120 seconds'
    elapsed = round((time.perf_counter() - started) * 1000)
    return {
        'id': step_id,
        'command': command,
        'passed': return_code == 0,
        'returnCode': return_code,
        'durationMs': elapsed,
        'output': output[-20000:]
    }

def expand_command(parts: list[str], root: Path) -> list[str]:
    mapping = {'{python}': sys.executable, '{root}': str(root)}
    return [mapping.get(part, part.replace('{root}', str(root))) for part in parts]

def platform_id() -> str:
    machine = platform.machine().lower()
    if sys.platform.startswith('win'):
        return 'windows-x64' if machine in ('amd64','x86_64') else f'windows-{machine}'
    if sys.platform == 'darwin':
        return 'macos-arm64' if machine in ('arm64','aarch64') else f'macos-{machine}'
    return 'linux-arm64' if machine in ('arm64','aarch64') else 'linux-x64'

def write_markdown(result: dict, path: Path) -> None:
    lines = [
        '# Venom Release Certification', '',
        f"**Version:** {result['product']['version']}  ",
        f"**Result:** {'PASS' if result['passed'] else 'FAIL'}  ",
        f"**Platform:** {result['environment']['platformId']}  ",
        f"**Generated:** {result['generatedAt']}  ", '',
        '## Gate summary', '',
        '| Gate | Result | Duration |', '|---|---:|---:|'
    ]
    for step in result['staticGates']:
        lines.append(f"| `{step['id']}` | {'PASS' if step['passed'] else 'FAIL'} | {step['durationMs']} ms |")
    lines += ['', '## Example qualification', '', '| Example | Profile | Build | Runtime | Leak scan |', '|---|---|---:|---:|---:|']
    for item in result['examples']:
        def status(value):
            if value is None: return 'N/A'
            return 'PASS' if value else 'FAIL'
        lines.append(f"| `{item['id']}` | {item['profile']} | {status(item.get('buildPassed'))} | {status(item.get('runtimeVerified'))} | {status(item.get('leakScanPassed'))} |")
    lines += ['', '## Browser evidence', '']
    if result['browserEvidence']:
        for report in result['browserEvidence']:
            lines.append(f"- `{report['path']}` — {'PASS' if report['passed'] else 'FAIL'}")
    else:
        lines.append('- Browser evidence was not supplied for this platform run.')
    lines += ['', '## Certified claims', '']
    lines.extend(f"- {claim}" for claim in result['claims'])
    lines += ['', '> A release is fully certified only when platform evidence from every required platform and browser evidence for every required browser have been aggregated by CI.', '']
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('\n'.join(lines), encoding='utf-8')

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--contract', type=Path, default=Path('contracts/release-certification.json'))
    ap.add_argument('--venom', type=Path)
    ap.add_argument('--output-dir', type=Path, default=Path('build/release-certification'))
    ap.add_argument('--browser-report', type=Path, action='append', default=[])
    ap.add_argument('--static-only', action='store_true')
    ap.add_argument('--keep-dists', action='store_true')
    args = ap.parse_args()
    root = args.repo_root.resolve()
    contract_path = args.contract if args.contract.is_absolute() else root / args.contract
    output = args.output_dir if args.output_dir.is_absolute() else root / args.output_dir
    contract = json.loads(contract_path.read_text(encoding='utf-8'))
    if contract.get('schema') != 'VENOM_RELEASE_CERTIFICATION_V1':
        raise SystemExit('unsupported release certification contract')
    version = version_from_cmake(root)
    env = os.environ.copy()
    env.setdefault('SOURCE_DATE_EPOCH', '1704067200')
    static_results = []
    for gate in contract['staticGates']:
        static_results.append(run_step(gate['id'], expand_command(gate['command'], root), root, env))
    example_results = []
    venom = args.venom.resolve() if args.venom else None
    dist_root = output / 'distributions'
    if not args.static_only:
        if not venom or not venom.is_file():
            raise SystemExit('--venom is required unless --static-only is used')
        for index, spec in enumerate(contract['examples']):
            site = root / spec['site']; dist = dist_root / spec['id']
            if dist.exists(): shutil.rmtree(dist)
            command = [str(venom), 'build', str(site), '--out', str(dist), '--profile', spec['profile']]
            if spec['profile'] == 'prod': command += ['--seed', str(1600000 + index)]
            build = run_step(f"example:{spec['id']}:build", command, root, env)
            runtime = None; leak = None; details = [build]
            if build['passed'] and spec.get('verifyRuntime'):
                check = run_step(f"example:{spec['id']}:runtime", [str(venom), 'verify-runtime', str(dist), '--target', 'browser', '--require-real-engine'], root, env)
                runtime = check['passed']; details.append(check)
            if build['passed'] and spec.get('leakScan'):
                check = run_step(f"example:{spec['id']}:leaks", [sys.executable, str(root/'tools/check_production_leaks.py'), str(dist)], root, env)
                leak = check['passed']; details.append(check)
            example_results.append({'id': spec['id'], 'profile': spec['profile'], 'required': spec.get('required', True), 'buildPassed': build['passed'], 'runtimeVerified': runtime, 'leakScanPassed': leak, 'steps': details})
    browser_evidence = []
    for report_path in args.browser_report:
        path = report_path if report_path.is_absolute() else root / report_path
        data = json.loads(path.read_text(encoding='utf-8'))
        passed = data.get('passed') is True or all(x.get('passed') is True for x in data.get('results', []))
        browser_evidence.append({'path': str(path), 'sha256': sha256(path), 'passed': passed})
    required_examples_pass = all(
        item['buildPassed'] and item.get('runtimeVerified') is not False and item.get('leakScanPassed') is not False
        for item in example_results if item['required']
    ) if example_results else args.static_only
    passed = all(x['passed'] for x in static_results) and required_examples_pass and all(x['passed'] for x in browser_evidence)
    result = {
        'schema': SCHEMA,
        'generatedAt': datetime.now(timezone.utc).isoformat(),
        'passed': passed,
        'product': {'name': 'Venom', 'version': version},
        'contract': {'path': str(contract_path.relative_to(root)), 'sha256': sha256(contract_path), 'version': contract['version']},
        'environment': {'platformId': platform_id(), 'system': platform.platform(), 'python': platform.python_version(), 'sourceDateEpoch': env['SOURCE_DATE_EPOCH']},
        'requiredPlatforms': contract['requiredPlatforms'],
        'requiredBrowsers': contract['requiredBrowsers'],
        'staticGates': static_results,
        'examples': example_results,
        'browserEvidence': browser_evidence,
        'claims': contract['releaseClaims']
    }
    output.mkdir(parents=True, exist_ok=True)
    (output/'certification.json').write_text(json.dumps(result, indent=2, sort_keys=True)+'\n', encoding='utf-8')
    write_markdown(result, output/'certification.md')
    if dist_root.exists() and not args.keep_dists:
        shutil.rmtree(dist_root)
    print(json.dumps({'passed': passed, 'json': str(output/'certification.json'), 'markdown': str(output/'certification.md')}, indent=2))
    return 0 if passed else 70
if __name__ == '__main__':
    raise SystemExit(main())
