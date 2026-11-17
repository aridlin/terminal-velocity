#!/usr/bin/env python3
"""Prepare additional recorded feedback from the credited rubberduck CC0 packs."""
import argparse,hashlib,json,pathlib,subprocess,tempfile,zipfile
p=argparse.ArgumentParser();p.add_argument('source_directory',type=pathlib.Path);args=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]/'assets/audio'
manifest=json.loads((root/'manifest.json').read_text())
clips=[
 ('door','100-cc0-sfx.zip','door_open.ogg','https://opengameart.org/content/100-cc0-sfx',1.2),
 ('steam','100-cc0-sfx-2.zip','sfx100v2_air_02.ogg','https://opengameart.org/content/100-cc0-sfx-2',1.1),
 ('lift','100-cc0-sfx.zip','machine_02.ogg','https://opengameart.org/content/100-cc0-sfx',.8),
 ('cloth','100-cc0-sfx.zip','paper_02.ogg','https://opengameart.org/content/100-cc0-sfx',.65),
]
with tempfile.TemporaryDirectory(prefix='tv-audio-') as tmp:
 for name,archive,member,url,duration in clips:
  with zipfile.ZipFile(args.source_directory/archive) as z:data=z.read(member)
  source=pathlib.Path(tmp)/member;source.write_bytes(data);out=root/(name+'.wav')
  filters=f'atrim=0:{duration},highpass=f=65,lowpass=f=7000,loudnorm=I=-22:TP=-7:LRA=7,afade=t=in:d=0.004,afade=t=out:st={duration-.08}:d=0.08'
  subprocess.run(['ffmpeg','-v','error','-y','-i',str(source),'-af',filters,'-ar','24000','-ac','1',str(out)],check=True)
  manifest=[m for m in manifest if m['file']!=out.name]
  manifest.append(dict(file=out.name,source=member,source_url=url,author='rubberduck',license='CC0-1.0',source_sha256=hashlib.sha256(data).hexdigest(),changes=f'Recorded excerpt up to {duration}s; 65 Hz high-pass and 7 kHz low-pass; -22 LUFS target, -7 dBTP ceiling; short edge fades; 24 kHz mono PCM.',sha256=hashlib.sha256(out.read_bytes()).hexdigest()))
(root/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Prepared four additional recorded feedback clips.')
