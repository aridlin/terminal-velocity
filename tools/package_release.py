#!/usr/bin/env python3
"""Package already built/verified binaries and the exact committed source tree.
Usage: python tools/package_release.py DESTINATION [--windows]
No builds, downloads, profile changes or installation are performed.
"""
import argparse
from datetime import datetime, timezone
import hashlib
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args], text=True).strip()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--windows', action='store_true')
    args = parser.parse_args()
    if git('status', '--porcelain', '--untracked-files=normal'):
        raise SystemExit('Commit the reviewed source before packaging; working tree is not clean.')
    revision = git('rev-parse', 'HEAD')
    version = revision[:12]
    dest = args.destination.resolve()
    dest.mkdir(parents=True, exist_ok=True)
    variants = [('linux-x64', ['terminal_velocity', 'terminal_velocity_sdl', 'keyboard_launcher', 'play.sh', 'ghostty-play.sh'])]
    if args.windows:
        variants.append(('windows-x64-window', ['terminal_velocity.exe', 'terminal_velocity_sdl.exe', 'SDL2.dll', 'play.cmd', 'play-window.cmd']))
    for _, files in variants:
        for name in files:
            if not (ROOT / name).is_file():
                raise SystemExit(f'Missing build input: {name}')
    products = []
    for platform, files in variants:
        package = f'terminal-velocity-{platform}-{version}'
        extension = '.zip' if platform.startswith('windows') else '.tar.gz'
        archive = dest / (package + extension)
        if archive.exists():
            raise SystemExit(f'Refusing to replace an existing artifact: {archive}')
        with tempfile.TemporaryDirectory(prefix='.tv-package-', dir=dest) as temporary:
            folder = Path(temporary) / package
            folder.mkdir()
            for name in files + ['README.md', 'README.pl.md', 'DEVELOPMENT.md', 'THIRD_PARTY.md']:
                shutil.copy2(ROOT / name, folder / name)
            for name in ['assets', 'docs']:
                shutil.copytree(ROOT / name, folder / name, ignore=shutil.ignore_patterns('media') if name == 'docs' else None)
            hashes = []
            for name in files:
                hashes.append(f'{hashlib.sha256((folder/name).read_bytes()).hexdigest()}  {name}')
            (folder / 'BUILD.txt').write_text(
                f'Terminal Velocity — Beneath the Surface\nCommit: {revision}\n'
                f'Platform: {platform}\nPackaged UTC: {datetime.now(timezone.utc).isoformat()}\n'
                'Built binaries must be verified separately; this file records their identity.\n\n' + '\n'.join(hashes) + '\n', encoding='utf-8')
            if extension == '.zip':
                with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as output:
                    for file in sorted(folder.rglob('*')):
                        if file.is_file():
                            output.write(file, file.relative_to(Path(temporary)))
            else:
                with tarfile.open(archive, 'x:gz') as output:
                    output.add(folder, arcname=package)
        products.append(archive)
    source = dest / f'terminal-velocity-source-{version}.tar.gz'
    if source.exists():
        raise SystemExit(f'Refusing to replace an existing artifact: {source}')
    subprocess.run(['git', '-C', str(ROOT), 'archive', '--format=tar.gz',
                    f'--prefix=terminal-velocity-source-{version}/', f'--output={source}', revision], check=True)
    products.append(source)
    checksum_file = dest / f'SHA256SUMS-{version}.txt'
    checksum_file.write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in products))
    for product in products + [checksum_file]:
        print(product)

if __name__ == '__main__':
    main()
