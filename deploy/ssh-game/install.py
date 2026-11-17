#!/usr/bin/env python3
"""Root-only staged installation. --publish follows successful loopback tests."""
import argparse, hashlib, json, os, pathlib, pwd, re, shutil, stat, subprocess, time
P=pathlib.Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('binary',type=P);p.add_argument('--publish',action='store_true');a=p.parse_args()
if os.geteuid()!=0:raise SystemExit('Run on the target host as root.')
source=P(__file__).resolve().parent
base=P('/opt/terminal-velocity');jail=base/'guest-root'
shell=P('/usr/local/libexec/terminal-velocity-guest')
conf=P('/etc/ssh/sshd_config.d/01-terminal-velocity-guest.conf')
stamp=time.strftime('%Y%m%d-%H%M%S');backup=base/'backups'/stamp
backup.mkdir(parents=True)
def run(*cmd,**kwargs):return subprocess.run(cmd,check=True,**kwargs)
def effective(user):return subprocess.check_output(['/usr/sbin/sshd','-T','-C',f'user={user},host=localhost,addr=127.0.0.1'],text=True)
before=effective('root');(backup/'admin-effective-before.txt').write_text(before)
if conf.exists():shutil.copy2(conf,backup/conf.name)
marker=base/'guest-account.json'
try:
 account=pwd.getpwnam('gra')
 if not marker.exists() or account.pw_shell!=str(shell):raise SystemExit('Refusing to change an unrelated existing gra account.')
except KeyError:account=None
if a.publish:
 proof=base/'guest-tested.json'
 if not proof.exists():raise SystemExit('Run the real loopback SSH tests before publishing.')
 verified=json.loads(proof.read_text())
 if verified.get('sha256')!=hashlib.sha256((jail/'game/terminal_velocity').read_bytes()).hexdigest():raise SystemExit('The tested binary changed; retest before publishing.')
 if not marker.exists() or not (jail/'game/terminal_velocity').is_file():raise SystemExit('Prepare and test the jail first.')
 conf.write_text('''# Only the anonymous game account; all other logins keep existing rules.
Match User gra
    AuthenticationMethods none
    PasswordAuthentication yes
    PermitEmptyPasswords yes
    PubkeyAuthentication no
    KbdInteractiveAuthentication no
    AuthorizedKeysFile none
    DisableForwarding yes
    PermitTunnel no
    PermitTTY yes
    PermitUserRC no
    MaxSessions 1
    ChrootDirectory /opt/terminal-velocity/guest-root
    ForceCommand /game/session
Match all
''')
else:
 if not a.binary.is_file():raise SystemExit('Missing verified game executable.')
 # Install the denial before unlocking the new account. Local test sshd has
 # an independent configuration and can authenticate it without public access.
 conf.write_text('Match User gra\n    DenyUsers gra\nMatch all\n')
 run('/usr/sbin/sshd','-t')
 if effective('root')!=before:raise SystemExit('Administrative sshd settings changed; stop before reload.')
 run('systemctl','reload','ssh.service')
 for folder in ['game','tmp','dev','etc','usr/local/libexec','var/lib/terminal-velocity/guest']:
  (jail/folder).mkdir(parents=True,exist_ok=True)
 os.chmod(jail,0o755);os.chmod(jail/'tmp',0o1777)
 shell.parent.mkdir(parents=True,exist_ok=True)
 shutil.copyfile(source/'guest-shell',shell);os.chmod(shell,0o755)
 shutil.copyfile(shell,jail/str(shell).lstrip('/'));os.chmod(jail/str(shell).lstrip('/'),0o755)
 shutil.copyfile(source/'guest-session',jail/'game/session');os.chmod(jail/'game/session',0o755)
 if (jail/'game/terminal_velocity').exists():shutil.copy2(jail/'game/terminal_velocity',backup/'terminal_velocity')
 shutil.copyfile(a.binary,jail/'game/terminal_velocity.next');os.chmod(jail/'game/terminal_velocity.next',0o755);os.replace(jail/'game/terminal_velocity.next',jail/'game/terminal_velocity')
 # Copy real files at the exact paths recorded by the loader, not host symlinks.
 binaries=[(a.binary,P('/game/terminal_velocity'))]+[(P(x),P(x)) for x in ['/bin/sh','/usr/bin/env','/usr/bin/mktemp','/usr/bin/rm','/usr/bin/timeout']]
 for binary,dest in binaries:
  if dest!=P('/game/terminal_velocity'):
   target=jail/str(dest).lstrip('/');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(binary,target);os.chmod(target,0o755)
  dependencies=subprocess.check_output(['ldd',str(binary)],text=True)
  if 'not found' in dependencies:raise SystemExit('Missing dynamic dependency')
  for dep in re.findall(r'(?:=>\s+)?(/[^\s()]+)',dependencies):
   target=jail/dep.lstrip('/');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(dep,target);os.chmod(target,0o755)
 for name,minor in [('null',3),('tty',0),('urandom',9)]:
  target=jail/'dev'/name
  if not target.exists():os.mknod(target,stat.S_IFCHR|0o666,os.makedev(5 if name=='tty' else 1,minor))
  os.chmod(target,0o666)
 if P('/usr/lib/locale/C.utf8').exists():shutil.copytree('/usr/lib/locale/C.utf8',jail/'usr/lib/locale/C.utf8',dirs_exist_ok=True)
 if account is None:
  run('useradd','--system','--user-group','--no-create-home','--home-dir','/var/lib/terminal-velocity/guest','--shell',str(shell),'gra');account=pwd.getpwnam('gra')
 (jail/'etc/passwd').write_text(f'gra:x:{account.pw_uid}:{account.pw_gid}:Terminal Velocity:/var/lib/terminal-velocity/guest:{shell}\n')
 (jail/'etc/group').write_text(f'gra:x:{account.pw_gid}:\n')
 (jail/'etc/nsswitch.conf').write_text('passwd: files\ngroup: files\nhosts: files\n')
 (jail/'var/lib/terminal-velocity/guest/.hushlogin').touch()
 home=P('/var/lib/terminal-velocity/guest');home.mkdir(parents=True,exist_ok=True);(home/'.hushlogin').touch()
 marker.write_text(json.dumps({'user':'gra','uid':account.pw_uid,'shell':str(shell),'created':stamp})+'\n')
 run('usermod','--password','','gra')
 # A loopback-only listener uses PAM just like the future public entry.
 local=(source/'sshd_config').read_text().replace('ListenAddress 0.0.0.0','ListenAddress 127.0.0.1').replace('ListenAddress ::\n','').replace('UsePAM no','UsePAM yes')
 P('/etc/ssh/terminal-velocity-sshd.conf').write_text(local)
 shutil.copyfile(source/'terminal-velocity-ssh.service','/etc/systemd/system/terminal-velocity-ssh.service')
 run('systemctl','daemon-reload');run('systemctl','restart','terminal-velocity-ssh.service')
 run('systemctl','is-active','terminal-velocity-ssh.service')
# All changed options are scoped to the guest. Roll back the drop-in on failure.
try:
 run('/usr/sbin/sshd','-t')
 after=effective('root');(backup/'admin-effective-after.txt').write_text(after)
 if after!=before:raise RuntimeError('Administrative SSH settings changed')
 (backup/'guest-effective.txt').write_text(effective('gra'))
 run('systemctl','reload','ssh.service')
except Exception:
 old=backup/conf.name
 if old.exists():shutil.copyfile(old,conf)
 else:conf.unlink(missing_ok=True)
 run('/usr/sbin/sshd','-t');run('systemctl','reload','ssh.service');raise
print(('Published guest match on port 22.' if a.publish else 'Prepared loopback guest service; public guest login denied.')+' Backups: '+str(backup))
