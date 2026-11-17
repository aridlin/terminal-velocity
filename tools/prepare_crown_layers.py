#!/usr/bin/env python3
"""Build two phase-aligned atmospheric music layers from licensed recordings.
No synthesized notes. Both files share an eight-second 48 kHz transport.
"""
from pathlib import Path
import hashlib,json,subprocess,wave
import numpy as np
root=Path(__file__).resolve().parents[1];audio=root/'assets/audio';rate=48000;length=rate*8

def read(name):
 return np.frombuffer(subprocess.check_output(['ffmpeg','-v','error','-i',str(audio/name),'-ar',str(rate),'-ac','1','-f','f32le','-']),dtype='<f4').copy()
def place(out,source,start,gain):
 n=min(len(source),len(out)-start)
 if n<=0:return
 source=source[:n].copy();edge=min(960,n//2)
 source[:edge]*=np.linspace(0,1,edge);source[-edge:]*=np.linspace(1,0,edge)
 out[start:start+n]+=source*gain
rumble=read('rumble.wav');metal=read('metal1.wav');cloth=read('cloth.wav')
low=np.zeros(length);high=np.zeros(length)
# Slow overlapping machinery breaths, with sparse recorded contact sounds.
for start,gain in [(0,.38),(rate*2,.27),(rate*4,.35),(rate*6,.23)]:place(low,rumble,start,gain)
for start,gain in [(rate//2,.16),(rate*3,.11),(rate*4+rate//2,.19),(rate*7,.09)]:place(high,metal,start,gain)
for start in [rate,rate*5]:place(high,cloth,start,.12)
manifest=json.loads((audio/'manifest.json').read_text());byfile={x['file']:x for x in manifest}
for name,data,sources in [('crown-foundation.wav',low,['rumble.wav']),('crown-tension.wav',high,['metal1.wav','cloth.wav'])]:
 # Seam-safe silence at the loop seam; source envelopes remain intact.
 data[:2400]*=np.linspace(0,1,2400);data[-2400:]*=np.linspace(1,0,2400)
 assert np.max(np.abs(data))<.8
 path=audio/name
 with wave.open(str(path),'wb') as out:
  out.setnchannels(1);out.setsampwidth(2);out.setframerate(rate);out.writeframes((np.clip(data,-1,1)*32767).astype('<i2').tobytes())
 row={'file':name,'source':sources,'source_url':[byfile[x]['source_url'] for x in sources],
      'author':'; '.join(sorted(set(byfile[x]['author'] for x in sources))),
      'license':'; '.join(sorted(set(byfile[x]['license'] for x in sources))),
      'source_sha256':{x:hashlib.sha256((audio/x).read_bytes()).hexdigest() for x in sources},
      'changes':'Recorded machinery/foley arranged as an eight-second atmospheric layer, 48 kHz mono PCM; trimmed envelopes and loop-seam fades. Not an isolated musical stem.',
      'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
 manifest=[x for x in manifest if x['file']!=name]+[row]
 print(name,len(data),'frames',row['sha256'])
(audio/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
