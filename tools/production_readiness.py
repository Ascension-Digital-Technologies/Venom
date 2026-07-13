#!/usr/bin/env python3
"""Generate a conservative production-readiness report for a Venom project.

This tool intentionally orchestrates existing Venom checks rather than redefining
package or runtime acceptance rules. It can inspect source-only projects or a
source + generated distribution pair and emits stable text or JSON output.
"""
from __future__ import annotations
import argparse, hashlib, json, os, shutil, subprocess, sys
from pathlib import Path

SCHEMA_VERSION = 1


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def run(cmd: list[str], cwd: Path | None = None) -> tuple[int, str]:
    p = subprocess.run(cmd, cwd=cwd, text=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, errors='replace')
    return p.returncode, p.stdout.strip()


def add(checks: list[dict], check_id: str, status: str, summary: str, detail: str = '') -> None:
    checks.append({'id': check_id, 'status': status, 'summary': summary, 'detail': detail})


def main() -> int:
    ap = argparse.ArgumentParser(description='Assess a Venom source project and optional distribution.')
    ap.add_argument('site', type=Path, help='source website directory')
    ap.add_argument('--dist', type=Path, help='generated Venom distribution directory')
    ap.add_argument('--venom', default='venom', help='path to venom executable')
    ap.add_argument('--format', choices=('text', 'json'), default='text')
    ap.add_argument('--require-real-engine', action='store_true', help='require a production QuickJS/WASM runtime')
    ap.add_argument('--strict', action='store_true', help='treat warnings as release blockers')
    ap.add_argument('--browser-report', type=Path, help='browser-validation JSON report for this distribution')
    ns = ap.parse_args()

    site = ns.site.resolve()
    dist = ns.dist.resolve() if ns.dist else None
    venom = shutil.which(ns.venom) or (str(Path(ns.venom).resolve()) if Path(ns.venom).exists() else None)
    checks: list[dict] = []

    if not site.is_dir():
        add(checks, 'source.exists', 'fail', 'Source website directory is missing.', str(site))
    else:
        add(checks, 'source.exists', 'pass', 'Source website directory exists.', str(site))
        indexes = [site / 'index.html', site / 'index.htm']
        if any(p.is_file() for p in indexes):
            add(checks, 'source.entry', 'pass', 'A root HTML entry document is present.')
        else:
            add(checks, 'source.entry', 'warn', 'No root index.html or index.htm was found.', 'Multi-route projects may still build when configured explicitly.')

    if venom:
        add(checks, 'tool.venom', 'pass', 'Venom executable is available.', venom)
        rc, out = run([venom, '--version'])
        add(checks, 'tool.version', 'pass' if rc == 0 else 'fail', 'Venom version command succeeded.' if rc == 0 else 'Venom version command failed.', out)
        if site.is_dir():
            rc, out = run([venom, 'compatibility', 'check', str(site), '--format', 'json'])
            status = 'pass' if rc == 0 else ('fail' if rc == 20 else 'fail')
            detail = out
            try:
                parsed = json.loads(out)
                findings = parsed.get('findings', [])
                partial = sum(1 for f in findings if f.get('status') == 'partial')
                blocked = sum(1 for f in findings if f.get('status') in ('denied', 'unsupported'))
                summary = f'Compatibility preflight found {blocked} blocking and {partial} partial feature(s).'
                if blocked == 0 and partial:
                    status = 'warn'
                elif blocked == 0:
                    status = 'pass'
            except Exception:
                summary = 'Compatibility preflight completed.' if rc in (0, 20) else 'Compatibility preflight failed to run.'
            add(checks, 'source.compatibility', status, summary, detail)
    else:
        add(checks, 'tool.venom', 'fail', 'Venom executable was not found.', ns.venom)

    if dist is None:
        add(checks, 'dist.supplied', 'warn', 'No generated distribution was supplied.', 'Use --dist to run runtime and deployment checks.')
    elif not dist.is_dir():
        add(checks, 'dist.exists', 'fail', 'Distribution directory is missing.', str(dist))
    else:
        add(checks, 'dist.exists', 'pass', 'Distribution directory exists.', str(dist))
        index = dist / 'index.html'
        assets = dist / 'assets'
        add(checks, 'dist.index', 'pass' if index.is_file() else 'fail', 'Generated index.html is present.' if index.is_file() else 'Generated index.html is missing.')
        add(checks, 'dist.assets', 'pass' if assets.is_dir() else 'fail', 'Generated assets directory is present.' if assets.is_dir() else 'Generated assets directory is missing.')

        provenance = dist / 'assets' / 'app' / 'build.json'
        if provenance.is_file():
            try:
                data = json.loads(provenance.read_text(encoding='utf-8'))
                required = ('venom_version', 'package_sha256')
                missing = [k for k in required if not data.get(k)]
                add(checks, 'dist.provenance', 'pass' if not missing else 'fail',
                    'Build provenance is present and parseable.' if not missing else 'Build provenance is missing required fields.',
                    ', '.join(missing) if missing else sha256(provenance))
            except Exception as exc:
                add(checks, 'dist.provenance', 'fail', 'Build provenance is not valid JSON.', str(exc))
        else:
            add(checks, 'dist.provenance', 'fail', 'assets/app/build.json is missing.')

        wasm = sorted(assets.rglob('*.wasm')) if assets.is_dir() else []
        packages = sorted(assets.rglob('*.vbc')) if assets.is_dir() else []
        add(checks, 'dist.wasm', 'pass' if wasm else 'fail', 'WebAssembly runtime asset is present.' if wasm else 'No WebAssembly runtime asset was found.')
        add(checks, 'dist.package', 'pass' if len(packages) == 1 else 'fail', 'Exactly one protected application package is present.' if len(packages) == 1 else f'Expected one .vbc package; found {len(packages)}.')

        if venom:
            verify = [venom, 'verify-runtime', str(dist), '--target', 'browser']
            if ns.require_real_engine:
                verify.append('--require-real-engine')
            rc, out = run(verify)
            add(checks, 'dist.runtime-verification', 'pass' if rc == 0 else 'fail',
                'Runtime and package verification passed.' if rc == 0 else 'Runtime or package verification failed.', out)

        cache_mixing_risk = any(p.name.lower() in ('service-worker.js', 'sw.js') for p in dist.rglob('*.js'))
        add(checks, 'deployment.atomic', 'warn',
            'Deploy all generated files atomically; mixed-build assets are rejected.',
            'A service worker is present and cache invalidation must be tested.' if cache_mixing_risk else 'Use immutable caching only for hashed assets and revalidate index.html.')
        add(checks, 'deployment.mime', 'warn', 'The production server must serve .wasm as application/wasm.', 'This cannot be proven from static files alone.')
        add(checks, 'deployment.https', 'warn', 'Production deployment should use HTTPS and security headers.', 'This cannot be proven from static files alone.')

        if ns.browser_report:
            report_path = ns.browser_report.resolve()
            if not report_path.is_file():
                add(checks, 'browser.validation', 'fail', 'The supplied browser validation report is missing.', str(report_path))
            else:
                try:
                    report = json.loads(report_path.read_text(encoding='utf-8'))
                    report_digest = report.get('dist_sha256')
                    actual_digest = None
                    digest_fn = hashlib.sha256()
                    for path in sorted(p for p in dist.rglob('*') if p.is_file()):
                        rel = path.relative_to(dist).as_posix().encode('utf-8')
                        digest_fn.update(len(rel).to_bytes(4, 'big'))
                        digest_fn.update(rel)
                        digest_fn.update(path.read_bytes())
                    actual_digest = digest_fn.hexdigest()
                    browsers = report.get('results', [])
                    passed = report.get('passed') is True and browsers and all(item.get('passed') is True for item in browsers)
                    same_dist = report_digest == actual_digest
                    status = 'pass' if passed and same_dist else 'fail'
                    summary = 'Real-browser validation passed for the supplied distribution.' if status == 'pass' else 'Real-browser validation did not prove this distribution passed.'
                    detail = f"browsers={','.join(str(item.get('browser')) for item in browsers)}; report_dist_match={'yes' if same_dist else 'no'}"
                    add(checks, 'browser.validation', status, summary, detail)
                except Exception as exc:
                    add(checks, 'browser.validation', 'fail', 'Browser validation report is invalid.', str(exc))
        else:
            add(checks, 'browser.validation', 'warn', 'No real-browser validation report was supplied.', 'Run scripts/browser-validate.* and pass --browser-report for release evidence.')

    blockers = sum(1 for c in checks if c['status'] == 'fail')
    warnings = sum(1 for c in checks if c['status'] == 'warn')
    ready = blockers == 0 and (warnings == 0 if ns.strict else True)
    result = {
        'schema_version': SCHEMA_VERSION,
        'ready': ready,
        'strict': ns.strict,
        'blockers': blockers,
        'warnings': warnings,
        'site': str(site),
        'dist': str(dist) if dist else None,
        'checks': checks,
    }

    if ns.format == 'json':
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print('Venom production readiness report')
        print(f'Site: {site}')
        if dist: print(f'Distribution: {dist}')
        print()
        for c in checks:
            label = {'pass':'PASS','warn':'WARN','fail':'FAIL'}[c['status']]
            print(f'[{label}] {c["id"]}: {c["summary"]}')
            if c['detail']:
                print(f'       {c["detail"]}')
        print(f'\nResult: {"READY" if ready else "NOT READY"} ({blockers} blocker(s), {warnings} warning(s))')

    return 0 if ready else 60

if __name__ == '__main__':
    raise SystemExit(main())
