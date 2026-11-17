#!/usr/bin/env python3
"""Capture the real SDL Unicode window at initial, resized, zoomed and fullscreen sizes.
Runs on an isolated X server/profile; never opens or writes the player's save.
"""
import os,pathlib,subprocess,tempfile,time,sys
root=pathlib.Path(__file__).resolve().parents[1]
out=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root.parent/'frontend-evidence'
out.mkdir(parents=True,exist_ok=True)
desktop='--desktop' in sys.argv
display=None if desktop else next(n for n in range(80,100) if not pathlib.Path(f'/tmp/.X11-unix/X{n}').exists())
xvfb=None if desktop else subprocess.Popen(['Xvfb',f':{display}','-screen','0','1920x1080x24','-nolisten','tcp'],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
app=None
try:
 for _ in range(0 if desktop else 100):
  if pathlib.Path(f'/tmp/.X11-unix/X{display}').exists():break
  if xvfb.poll() is not None:raise RuntimeError(xvfb.stderr.read().decode())
  time.sleep(.02)
 with tempfile.TemporaryDirectory(prefix='tv-sdl-view-') as private:
  env=dict(os.environ,DISPLAY=os.environ.get('DISPLAY',':0') if desktop else f':{display}',SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',TV_ICEBERG_FILE=str(pathlib.Path(private)/'profile.iceberg'))
  def command(*args):return subprocess.check_output(args,env=env,text=True).strip()
  app=subprocess.Popen([str(root/'terminal_velocity_sdl'),'--terminal-view','--mute','--language','en'],cwd=root,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
  window=None
  for _ in range(300):
   try:window=command('xdotool','search','--all','--onlyvisible','--pid',str(app.pid),'--name','Terminal Velocity').splitlines()[0];break
   except subprocess.CalledProcessError:
    if app.poll() is not None:raise RuntimeError(app.stderr.read().decode())
    time.sleep(.02)
  assert window,'SDL window not found'
  time.sleep(.5)
  window=command('xdotool','search','--all','--onlyvisible','--pid',str(app.pid),'--name','Terminal Velocity').splitlines()[-1]
  
  if desktop:command('xdotool','windowactivate','--sync',window)
  command('xdotool','windowfocus',window);command('xdotool','key','--window',window,'Return');time.sleep(.8)
  def capture(name):
   geometry=command('xdotool','getwindowgeometry','--shell',window)
   command('import','-window',window,str(out/(name+'.png')))
   print('CAPTURE',name,geometry.replace('\n',' '))
  capture('sdl-initial')
  for name,w,h in [('sdl-resized-small',760,520),('sdl-resized-wide',1500,760)]:
   command('xdotool','windowsize',window,str(w),str(h));time.sleep(.15);capture(name)
  if desktop:
   command('xdotool','key','--window',window,'F11');time.sleep(.3);capture('sdl-fullscreen')
   geometry=command('xdotool','getwindowgeometry','--shell',window);assert '_NET_WM_STATE_FULLSCREEN' in command('xprop','-id',window,'_NET_WM_STATE'),geometry
   command('xdotool','key','--window',window,'F11');time.sleep(.15)
  else:print('SKIP fullscreen: Xvfb has no window manager; use --desktop for real fullscreen')
  command('xdotool','key','--window',window,'ctrl+plus','ctrl+plus','ctrl+plus','ctrl+plus','ctrl+plus','ctrl+plus','ctrl+plus','ctrl+plus','ctrl+plus','ctrl+plus');time.sleep(.15);capture('sdl-zoomed')
  command('xdotool','key','--window',window,'ctrl+0');time.sleep(.1);capture('sdl-zoom-reset')
  command('xdotool','key','--window',window,'q');app.wait(timeout=5);assert app.returncode==0,app.stderr.read().decode()
  print('PASS real SDL initial, resized, zoom and reset window capture'+('; desktop fullscreen verified' if desktop else ''))
finally:
 if app and app.poll() is None:app.terminate();app.wait(timeout=5)
 if xvfb:xvfb.terminate();xvfb.wait(timeout=5)
