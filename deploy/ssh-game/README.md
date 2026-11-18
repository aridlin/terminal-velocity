# ProLiant guest entrance

Published on ProLiant and verified through the public relay on 17 November 2026. Real tests cover passwordless PTYs, independent visits, movement/inspection/resize/exit, and command/SFTP/local+remote forwarding denial. The installer compares administrative effective SSH settings before reloading. Re-run staging verification whenever changing the jail or binary.

Command: `ssh -t gra@prol.aridlin.pl`. No password or client key is required. OpenSSH's `none` authentication is limited to the dedicated `gra` account in an account-specific block on the existing listener. All other users retain their effective settings. A separate loopback-only listener on port 2223 is used to verify the guest before publishing.

The account's root-owned login shell ignores all supplied arguments and runs the game. The server also forces that command, prohibits forwarding, user startup files, environment injection and subsystems, and confines the session to a root-owned chroot containing only the game and its required libraries/utilities. It needs a PTY; noninteractive command requests without one exit. Each visit gets a temporary profile, removed on exit. Anonymous visits do not share campaign saves. A visit lasts at most two hours; reconnecting starts another visit. Quiet SSH input supports ordinary terminals and enhanced keyboard protocols.

The SSH menu uses the website's account service and syncs the primary campaign file while playing and when returning to the menu with Q. If two clients write different saves, the account menu prompts for the version to keep. The two extra local slots are scoped to the current SSH visit.

Build the menu with `CGO_ENABLED=0 go build -trimpath -o account-menu deploy/ssh-game/account-menu.go`. Copy the binary, `guest-session` and `install-accounts.py` to a private staging directory on ProLiant. Run `python3 install-accounts.py stage ./account-menu` and test `/game/session-accounts` as the guest user inside the jail. Run `python3 install-accounts.py publish` to switch new SSH visits over. Restore the printed backup wrapper to roll back the menu.

Installation and upgrade procedure:

1. Build and test the committed native terminal target on the server's distribution. Do not copy a binary whose dynamic libraries are unavailable there.
2. Create `/opt/terminal-velocity/guest-root`, owned by root with no group/other write permission. Install the game at `/game/terminal_velocity` inside it. Install `guest-session` at `/game/session` and mirror `guest-shell` at `/usr/local/libexec/terminal-velocity-guest` both outside and inside the jail, root-owned 0755.
3. Populate the jail with `/bin/sh`, `/usr/bin/env`, `/usr/bin/mktemp`, `/usr/bin/rm`, `/usr/bin/timeout` and their resolved dynamic dependencies. Add the game's dependencies, a UTF-8 locale, `/dev/null`, `/dev/tty` and a 1777 `/tmp`. Never copy host credentials or configuration into the jail.
4. Create the dedicated `gra` user with the restricted shell, home `/var/lib/terminal-velocity/guest`, no supplementary groups and an empty password. Refuse to modify an already existing unrelated account. Add `DenyUsers gra` to a backed-up administrative-sshd drop-in; validate and reload that daemon without restarting active sessions.
5. Copy the separate config and unit. Validate `sshd -t` and effective settings for `gra`, and prove normal administrative authentication remains unchanged. Start the guest listener on loopback first. Test a genuine passwordless PTY session, input, resize, inspection, quit/disconnect and simultaneous independent players. Verify command, SFTP and local/remote forwarding denial.
6. Run `install.py VERIFIED_BINARY --publish` only after those checks. It replaces the temporary guest denial with the restricted guest block on the existing port 22. Verify a real external passwordless connection through the relay. Preserve configuration backups and the previous release. Do not alter unrelated routes or firewall rules to guess a working path.

Rollback: restore the backed-up guest drop-in (or replace it with a guest-only denial), validate and reload the main sshd, stop/disable `terminal-velocity-ssh`, and restore its previous jail/release and configuration. Remove the dedicated user only after confirming no guest processes remain. A user and empty password must never outlive its restrictive login shell.

The configuration follows the [OpenSSH server manual](https://man.openbsd.org/sshd_config), especially `AuthenticationMethods`, `ChrootDirectory`, `ForceCommand` and `DisableForwarding`.
