#!/usr/bin/env python3
"""Exercise the restricted entrance through real passwordless SSH PTYs."""
import pyte, signal
import argparse, codecs, fcntl, hashlib, json, os, pathlib, pty, re, select, struct, subprocess, termios, time
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--host',default='127.0.0.1');p.add_argument('--port',type=int,default=2223)
p.add_argument('--known-hosts',required=True);p.add_argument('--record',action='store_true');a=p.parse_args()
base=['ssh','-F','/dev/null','-p',str(a.port),'-o','BatchMode=yes','-o','PreferredAuthentications=none','-o','PubkeyAuthentication=no','-o','PasswordAuthentication=no','-o','KbdInteractiveAuthentication=no','-o','StrictHostKeyChecking=yes','-o','UserKnownHostsFile='+a.known_hosts,'-o','ConnectTimeout=5']
target='gra@'+a.host
ansi=re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')
class Visit:
 def __init__(self):
  self.fd,slave=pty.openpty();self.slave=slave;self.decoder=codecs.getincrementaldecoder('utf-8')('replace');self.text='';self.screen=pyte.Screen(110,34);self.stream=pyte.Stream(self.screen)
  fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',34,110,1100,680))
  self.proc=subprocess.Popen(base+['-tt',target,'printf TV_ESCAPE_CHECK'],stdin=slave,stdout=slave,stderr=slave,start_new_session=True)
 def read(self,seconds=.15):
  end=time.monotonic()+seconds;new=''
  while time.monotonic()<end:
   if select.select([self.fd],[],[],.02)[0]:
    try:new+=self.decoder.decode(os.read(self.fd,262144))
    except OSError:break
  self.stream.feed(new);self.text='\n'.join(self.screen.display);return self.text
 def expect(self,word,seconds=5):
  end=time.monotonic()+seconds;seen=''
  while time.monotonic()<end:
   seen+=self.read(.1)
   if word in seen:return
   if self.proc.poll() is not None:break
  raise RuntimeError('Missing game frame: '+word+'; exit='+str(self.proc.poll())+'; tail='+repr(seen[-300:]))
 def send(self,value):os.write(self.fd,value.encode())
 def close(self):
  if self.proc.poll() is None:
   self.send('q');end=time.monotonic()+4
   while self.proc.poll() is None and time.monotonic()<end:self.read(.05)
  if self.proc.poll() is None:self.proc.terminate()
  self.proc.wait(timeout=3);os.close(self.fd);os.close(self.slave)
visits=[]
try:
 first=Visit();visits.append(first);first.expect('ROZPOCZNIJ PODRÓŻ')
 first.send('l');first.expect('BEGIN JOURNEY');first.send('\r');first.expect('LOWER HARBOR')
 first.send('d');first.expect('[E] Talk');first.read(.12)
 first.send('s');first.read(.05);first.send('b');first.expect('INSPECT');first.expect('HELENA')
 # A second connection is a new visitor, not the first visitor's saved journey.
 second=Visit();visits.append(second);second.expect('ROZPOCZNIJ PODRÓŻ')
 if 'TV_ESCAPE_CHECK' in first.text+second.text:raise RuntimeError('Client command escaped into a shell')
 first.screen.resize(24,76);fcntl.ioctl(first.slave,termios.TIOCSWINSZ,struct.pack('HHHH',24,76,760,480));os.kill(first.proc.pid,signal.SIGWINCH);first.expect('HELENA')
 first.send('b');first.read(.1)
 for visit in visits:visit.close()
 visits=[]
 def denied(args,phrase=None):
  result=subprocess.run(base+args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,timeout=8)
  if result.returncode==0:raise RuntimeError('Restricted SSH operation succeeded: '+repr(args))
  if phrase and phrase not in result.stderr:raise RuntimeError('Did not observe explicit forwarding refusal: '+result.stderr)
  if 'TV_ESCAPE_CHECK' in result.stdout:raise RuntimeError('Command was executed')
 denied(['-T',target,'printf TV_ESCAPE_CHECK'])
 denied(['-s',target,'sftp'])
 denied(['-W','127.0.0.1:22',target],'administratively prohibited')
 denied(['-o','ExitOnForwardFailure=yes','-N','-R','0:127.0.0.1:22',target],'remote port forwarding failed')
 if a.record:
  if os.geteuid()!=0 or a.host!='127.0.0.1' or a.port!=2223:raise RuntimeError('Record local root-controlled staging verification only')
  root=pathlib.Path('/opt/terminal-velocity')
  digest=hashlib.sha256((root/'guest-root/game/terminal_velocity').read_bytes()).hexdigest()
  (root/'guest-tested.json').write_text(json.dumps({'sha256':digest,'time':time.time(),'passwordless_pty':True,'isolated_visits':True,'resize_inspect_quit':True,'command_sftp_forwarding_denied':True})+'\n')
 print('PASS passwordless SSH, forced game despite supplied command, two independent visitors, movement/inspection/resize/exit, command/SFTP/forwarding denial')
finally:
 for visit in visits:
  try:visit.close()
  except Exception:pass
