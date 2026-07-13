#!/usr/bin/env python3
import pathlib, shutil, subprocess, sys, tempfile

venom = pathlib.Path(sys.argv[1]).resolve()
root = pathlib.Path(tempfile.mkdtemp(prefix='venom-image-layout-'))
try:
    site = root / 'site'; dist = root / 'dist'
    (site / 'media').mkdir(parents=True)
    (site / 'index.html').write_text('<!doctype html><html><head><link rel="stylesheet" href="site.css"></head><body><img src="media/logo.svg"></body></html>', encoding='utf-8')
    (site / 'site.css').write_text('body{background-image:url("media/logo.svg")}', encoding='utf-8')
    (site / 'media' / 'logo.svg').write_text('<svg xmlns="http://www.w3.org/2000/svg" width="1" height="1"/>', encoding='utf-8')
    subprocess.run([str(venom), 'build', str(site), '--out', str(dist)], check=True)
    images = list((dist / 'assets' / 'images').glob('*.svg'))
    if len(images) != 1:
        raise SystemExit(f'expected one SVG under assets/images, found {images}')
    if list((dist / 'assets').glob('*.svg')):
        raise SystemExit('image leaked into assets root')
    css_files = list((dist / 'assets' / 'style').glob('*.css'))
    if not css_files or '../images/' not in css_files[0].read_text(encoding='utf-8'):
        raise SystemExit('compiled CSS did not reference ../images/')
finally:
    shutil.rmtree(root, ignore_errors=True)
