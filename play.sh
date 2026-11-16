#!/bin/sh
set -eu
cd -- "$(dirname -- "$(readlink -f -- "$0")")"
if [ ! -x ./terminal_velocity ]; then make terminal_velocity; fi
# SSH and explicitly selected terminal modes must never open the server keyboard.
if [ -n "${SSH_CONNECTION:-}" ] || [ -n "${SSH_TTY:-}" ]; then
    exec ./terminal_velocity --ssh "$@"
fi
for arg in "$@"; do
    case "$arg" in
        --relay|--ssh|--ssh-basic|--demo|--help|--preview|--smoke|--input-test|--input)
            exec ./terminal_velocity "$@" ;;
    esac
done
if [ ! -x ./keyboard_launcher ]; then make keyboard_launcher; fi
if ./terminal_velocity --input-test >/dev/null 2>&1; then
    exec ./terminal_velocity "$@"
fi
printf '\nBezpośrednia klawiatura dla Ghostty\n'
printf 'Gra potrzebuje dostępu do urządzenia evdev.\n'
printf 'Pomocnik otworzy klawiaturę i od razu porzuci uprawnienia administratora.\n'
printf 'Nie zmienia grup, reguł udev ani praw dostępu. Dostęp kończy się z grą.\n'
printf 'Hasło sudo wpisz tutaj — nie w czacie.\n\n'
exec sudo -- ./keyboard_launcher "$@"
