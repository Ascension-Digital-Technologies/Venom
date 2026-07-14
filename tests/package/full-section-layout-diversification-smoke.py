#!/usr/bin/env python3
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile


def run(cmd):
    return subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def package_for(dist: pathlib.Path) -> pathlib.Path:
    matches = list((dist / 'assets' / 'app').glob('app.*.vbc'))
    if len(matches) != 1:
        raise SystemExit(f'expected one package, found {len(matches)}')
    return matches[0]


def section_rows(venom: pathlib.Path, pkg: pathlib.Path):
    text = run([str(venom), 'inspect', str(pkg)]).stdout
    rows = []
    for line in text.splitlines():
        if re.search(r'\bname="', line) and 'offset=' in line:
            rows.append(line.strip())
    if len(rows) < 3:
        raise SystemExit('inspect output did not expose enough section rows')
    return rows


def main():
    if len(sys.argv) != 2:
        raise SystemExit('usage: full-section-layout-diversification-smoke.py <venom>')
    venom = pathlib.Path(sys.argv[1]).resolve()
    repo = pathlib.Path(__file__).resolve().parents[2]
    site = repo / 'tests' / 'fixtures' / 'sites' / 'no-script-site'
    with tempfile.TemporaryDirectory(prefix='venom-full-layout-') as td:
        root = pathlib.Path(td)
        layouts = []
        packages = []
        for i in range(4):
            out = root / f'build-{i}'
            run([str(venom), 'build', str(site), '--out', str(out)])
            pkg = package_for(out)
            packages.append(pkg.read_bytes())
            layouts.append(section_rows(venom, pkg))

        if len({p for p in packages}) != len(packages):
            raise SystemExit('independent builds produced duplicate package bytes')
        if len({tuple(x) for x in layouts}) != len(layouts):
            raise SystemExit('independent builds produced duplicate section layouts')

        first_rows = {layout[0] for layout in layouts}
        if len(first_rows) < 2:
            raise SystemExit('first package section remained anchored across builds')

        first_types = {re.sub(r'\s+offset=.*$', '', row) for row in first_rows}
        if len(first_types) < 2:
            raise SystemExit('first package section identity remained stable across builds')

    print('full section layout diversification smoke: PASS')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
