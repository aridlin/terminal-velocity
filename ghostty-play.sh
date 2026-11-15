#!/bin/sh
set -eu
cd -- "$(dirname -- "$(readlink -f -- "$0")")"
if [ -n "${SSH_CONNECTION:-}" ] || [ -n "${SSH_TTY:-}" ]; then exec ./play.sh --ssh "$@"; fi
for arg in "$@"; do
    case "$arg" in --ssh|--ssh-basic|--help|--input-test) exec ./play.sh "$@" ;; esac
done
exec ghostty --title='Terminal Velocity' --maximize=true --font-size=12 --window-inherit-font-size=false --window-width=110 --window-height=34 -e "$PWD/play.sh" "$@"
