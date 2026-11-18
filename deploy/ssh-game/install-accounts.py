#!/usr/bin/env python3
"""Stage or activate the account launcher in an existing, restricted guest jail."""

import argparse
import ipaddress
import os
from pathlib import Path
import re
import shutil
import tempfile
import time


def install(source: Path, destination: Path, mode: int) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=destination.parent, delete=False) as temporary:
        target = Path(temporary.name)
    try:
        shutil.copyfile(source, target)
        target.chmod(mode)
        target.replace(destination)
    finally:
        target.unlink(missing_ok=True)


def stage(jail: Path, launcher: Path) -> None:
    source = Path(__file__).resolve().parent
    addresses = []
    for line in Path("/etc/resolv.conf").read_text().splitlines():
        fields = line.split()
        if len(fields) >= 2 and fields[0] == "nameserver":
            try:
                addresses.append(str(ipaddress.ip_address(fields[1])))
            except ValueError:
                continue
    if not addresses:
        raise SystemExit("No usable DNS resolver was found for the guest jail.")
    with tempfile.NamedTemporaryFile(mode="w") as resolver:
        resolver.writelines(f"nameserver {address}\n" for address in addresses[:3])
        resolver.flush()
        install(Path(resolver.name), jail / "etc/resolv.conf", 0o644)
    settings = jail / "etc/nsswitch.conf"
    contents = settings.read_text()
    contents, replaced = re.subn(r"(?m)^hosts:.*$", "hosts: files dns", contents)
    with tempfile.NamedTemporaryFile(mode="w") as nsswitch:
        nsswitch.write(contents if replaced else contents + "\nhosts: files dns\n")
        nsswitch.flush()
        install(Path(nsswitch.name), settings, 0o644)
    install(Path("/etc/ssl/certs/ca-certificates.crt"),
            jail / "etc/ssl/certs/ca-certificates.crt", 0o644)
    install(launcher, jail / "game/account-menu-staged", 0o755)
    with tempfile.NamedTemporaryFile(mode="w") as wrapper:
        wrapper.write((source / "guest-session").read_text().replace(
            "/game/account-menu", "/game/account-menu-staged"))
        wrapper.flush()
        install(Path(wrapper.name), jail / "game/session-accounts", 0o755)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    command = parser.add_subparsers(dest="action", required=True)
    command.add_parser("stage").add_argument("launcher", type=Path)
    command.add_parser("publish")
    args = parser.parse_args()
    base = Path("/opt/terminal-velocity")
    jail = base / "guest-root"
    if os.geteuid() != 0 or jail.stat().st_uid != 0 or jail.stat().st_mode & 0o022:
        raise SystemExit("The existing jail must be owned and managed by root.")
    if args.action == "stage":
        stage(jail, args.launcher)
        return
    if not (jail / "game/account-menu-staged").is_file():
        raise SystemExit("Stage and test the account launcher first.")
    backup = base / "backups" / ("accounts-" + time.strftime("%Y%m%d-%H%M%S"))
    backup.mkdir(parents=True)
    shutil.copy2(jail / "game/session", backup / "session")
    if (jail / "game/account-menu").exists():
        shutil.copy2(jail / "game/account-menu", backup / "account-menu")
    install(jail / "game/account-menu-staged", jail / "game/account-menu", 0o755)
    install(Path(__file__).resolve().with_name("guest-session"), jail / "game/session", 0o755)
    print(f"Guest account menu installed. Previous session wrapper: {backup / 'session'}")


if __name__ == "__main__":
    main()
