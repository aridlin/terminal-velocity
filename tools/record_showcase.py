#!/usr/bin/env python3
"""Record real --demo PTY output, then render it with pyte/Pillow and ffmpeg.
Requires Python 3, pyte, Pillow, ffmpeg and a locally built terminal_velocity.
No desktop capture, input devices, authentication or remote connection is used.
"""
import codecs
import concurrent.futures
import fcntl
import json
import os
from pathlib import Path
import pty
import select
import struct
import subprocess
import termios
import time
import pyte
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/media'
COLS,ROWS,CW,CH=110,34,10,20
FONT='/usr/share/fonts/TTF/DejaVuSansMono.ttf'
if not Path(FONT).exists(): FONT='/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'
font=ImageFont.truetype(FONT,16)
cache={}

def record(name,args):
    master,slave=pty.openpty()
    fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',ROWS,COLS,COLS*CW,ROWS*CH))
    proc=subprocess.Popen([str(ROOT/'terminal_velocity'),'--cell-aspect','2.0','--demo',*args],stdin=slave,stdout=slave,stderr=slave,start_new_session=True)
    os.close(slave)
    start=time.monotonic();events=[];decoder=codecs.getincrementaldecoder('utf-8')('replace')
    try:
        while time.monotonic()-start<45:
            if select.select([master],[],[],.05)[0]:
                try: data=os.read(master,131072)
                except OSError: break
                if not data: break
                text=decoder.decode(data)
                if text:events.append([round(time.monotonic()-start,6),'o',text])
            elif proc.poll() is not None:break
        if proc.poll() is None:proc.terminate()
        proc.wait(timeout=5)
    finally:os.close(master)
    if proc.returncode:raise RuntimeError(f'{name}: game exited {proc.returncode}')
    header={'version':2,'width':COLS,'height':ROWS,'title':name+' — automated Terminal Velocity demo','env':{'TERM':'xterm-256color'}}
    with (OUT/(name+'.cast')).open('w') as f:
        f.write(json.dumps(header)+'\n')
        for e in events:f.write(json.dumps(e,ensure_ascii=False)+'\n')
    print(f'Recorded {name}: {events[-1][0]:.1f}s',flush=True)
    return name,events

def rgb(value,default):
    names={'black':'000000','white':'eeeeee','red':'cd0000','green':'00cd00','blue':'0000ee','yellow':'cdcd00','magenta':'cd00cd','cyan':'00cdcd','brown':'cdcd00'}
    value=names.get(value,value)
    try:return tuple(int(value[i:i+2],16) for i in (0,2,4)) if len(value)==6 else default
    except ValueError:return default

def paint(screen,canvas,last):
    for y in range(ROWS):
        for x in range(COLS):
            c=screen.buffer[y][x];key=(c.data,c.fg,c.bg,c.bold,c.reverse)
            if last.get((x,y))==key:continue
            last[x,y]=key
            if key not in cache:
                fg,bg=rgb(c.fg,(200,187,184)),rgb(c.bg,(25,20,24))
                if c.reverse:fg,bg=bg,fg
                tile=Image.new('RGB',(CW,CH),bg)
                draw=ImageDraw.Draw(tile)
                # Font fallback must not turn the game's Braille pixels into tofu.
                if len(c.data)==1 and 0x2800<=ord(c.data)<=0x28ff:
                    bits=ord(c.data)-0x2800
                    dots=((1,8),(2,16),(4,32),(64,128))
                    for row in range(4):
                        for col in range(2):
                            if bits & dots[row][col]:
                                dot_x,dot_y=col*5+2,row*5+2
                                draw.ellipse((dot_x-1,dot_y-1,dot_x+1,dot_y+1),fill=fg)
                elif c.data=='█': draw.rectangle((0,0,CW,CH),fill=fg)
                else: draw.text((0,0),c.data,font=font,fill=fg)
                cache[key]=tile
            canvas.paste(cache[key],(x*CW,y*CH))

def render(name,events):
    screen=pyte.Screen(COLS,ROWS);stream=pyte.Stream(screen)
    canvas=Image.new('RGB',(COLS*CW,ROWS*CH),(25,20,24));last={}
    end=max(.1,events[-1][0]-.15);fps=20
    command=['ffmpeg','-v','error','-y','-f','rawvideo','-pix_fmt','rgb24','-s',f'{COLS*CW}x{ROWS*CH}','-r',str(fps),'-i','-','-an','-c:v','libx264','-preset','fast','-crf','18','-pix_fmt','yuv420p','-movflags','+faststart',str(OUT/(name+'.mp4'))]
    enc=subprocess.Popen(command,stdin=subprocess.PIPE);index=0;snapped=False
    for frame in range(int(end*fps)):
        t=frame/fps
        while index<len(events) and events[index][0]<=t:
            stream.feed(events[index][2]);index+=1
        paint(screen,canvas,last)
        if not snapped and t>=6.5:
            canvas.save(OUT/(name+'.png'));snapped=True
        enc.stdin.write(canvas.tobytes())
    enc.stdin.close()
    if enc.wait():raise RuntimeError('ffmpeg encode failed')
    subprocess.run(['ffmpeg','-v','error','-y','-ss','2','-t','10','-i',str(OUT/(name+'.mp4')),'-filter_complex','fps=10,scale=880:-1:flags=lanczos,split[a][b];[a]palettegen=stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=3','-loop','0',str(OUT/(name+'.gif'))],check=True)
    print('Rendered '+name,flush=True)

def menu():
    data=subprocess.check_output([str(ROOT/'terminal_velocity'),'--preview','menu','--seed','73']).decode()
    screen=pyte.Screen(COLS,ROWS);pyte.Stream(screen).feed(data)
    canvas=Image.new('RGB',(COLS*CW,ROWS*CH));paint(screen,canvas,{})
    canvas.save(OUT/'level-menu.png')

if __name__=='__main__':
    OUT.mkdir(parents=True,exist_ok=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        recordings=list(pool.map(lambda item:record(*item),[('procedural',['--seed','73']),('jp2-event',['--event'])]))
    for name,events in recordings:render(name,events)
    menu()
