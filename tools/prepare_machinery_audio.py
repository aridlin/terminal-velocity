#!/usr/bin/env python3
"""Prepare recorded machinery responses from the credited CC0 source packs."""
import argparse,hashlib,json,pathlib,subprocess,tempfile,zipfile
p=argparse.ArgumentParser();p.add_argument('source_directory',type=pathlib.Path);args=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]/'assets/audio';manifest=json.loads((root/'manifest.json').read_text())
clips=[('motor','sfx100v2_loop_machine_02.ogg',2.8,'highpass=f=75,lowpass=f=2600'),('rumble','sfx100v2_loop_machine_04.ogg',2.5,'highpass=f=35,lowpass=f=180'),('spark','sfx100v2_metal_hit_02.ogg',.55,'highpass=f=1400,lowpass=f=7000')]
with tempfile.TemporaryDirectory(prefix='tv-machinery-') as tmp:
 with zipfile.ZipFile(args.source_directory/'100-cc0-sfx-2.zip') as z:
  for name,member,duration,band in clips:
   data=z.read(member);source=pathlib.Path(tmp)/member;source.write_bytes(data);out=root/(name+'.wav')
   filters=f'aloop=loop=3:size=240000,atrim=0:{duration},{band},loudnorm=I=-23:TP=-8:LRA=7,afade=t=in:d=0.025,afade=t=out:st={duration-.18}:d=0.18'
   subprocess.run(['ffmpeg','-v','error','-y','-i',str(source),'-af',filters,'-ar','24000','-ac','1',str(out)],check=True)
   manifest=[m for m in manifest if m['file']!=out.name]
   manifest.append(dict(file=out.name,source=member,source_url='https://opengameart.org/content/100-cc0-sfx-2',author='rubberduck',license='CC0-1.0',source_sha256=hashlib.sha256(data).hexdigest(),changes=f'Recorded excerpt/loop up to {duration}s; {band}; -23 LUFS / -8 dBTP; edge fades; 24 kHz mono PCM.',sha256=hashlib.sha256(out.read_bytes()).hexdigest()))
(root/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
