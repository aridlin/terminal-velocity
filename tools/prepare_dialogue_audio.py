#!/usr/bin/env python3
"""Create dialogue syllables from Kenney's CC0 recorded voice performances."""
import argparse,hashlib,json,pathlib,subprocess,tempfile,zipfile
p=argparse.ArgumentParser();p.add_argument('archive',type=pathlib.Path);args=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1];audio=root/'assets/audio'
manifest=json.loads((audio/'manifest.json').read_text())
# A brief vowel from a real performance, with character-specific pitch and tone.
clips=[('Female/1.ogg',.09,1.0,4600),('Male/1.ogg',.10,.88,3600),
       ('Male/go.ogg',.08,.96,3900),('Female/go.ogg',.08,.92,4300),
       ('Female/level.ogg',.09,.86,3800),('Female/2.ogg',.10,1.04,4900),
       ('Male/level.ogg',.08,1.13,4800),('Male/1.ogg',.10,.73,1700)]
with zipfile.ZipFile(args.archive) as archive,tempfile.TemporaryDirectory(prefix='tv-voice-') as tmp:
 (audio/'KENNEY-VOICE-LICENSE.txt').write_bytes(archive.read('License.txt'))
 for i,(member,start,pitch,cutoff) in enumerate(clips):
  data=archive.read(member);source=pathlib.Path(tmp)/'source.ogg';source.write_bytes(data)
  destination=audio/f'talk{i}.wav'
  effects=f'silenceremove=start_periods=1:start_duration=0.01:start_threshold=-40dB,atrim=start={start}:duration=0.085,asetpts=PTS-STARTPTS,aresample=24000,asetrate={int(24000*pitch)},aresample=24000,highpass=f=130,lowpass=f={cutoff},loudnorm=I=-24:TP=-9:LRA=5,afade=t=in:d=0.006,areverse,afade=t=in:d=0.012,areverse'
  subprocess.run(['ffmpeg','-v','error','-y','-i',str(source),'-af',effects,'-ar','24000','-ac','1',str(destination)],check=True)
  manifest=[m for m in manifest if m['file']!=destination.name]
  manifest.append(dict(file=destination.name,source=member,source_url='https://lpc.opengameart.org/content/voiceover-pack-40-lines',author='Kenney',license='CC0-1.0',source_sha256=hashlib.sha256(data).hexdigest(),changes=f'Recorded vocal excerpt after leading silence, offset {start}s, duration85ms; pitch ratio{pitch}; 130Hz high-pass/{cutoff}Hz low-pass; -24LUFS/-9dBTP target;6/12ms edge fades;24kHz mono PCM.',sha256=hashlib.sha256(destination.read_bytes()).hexdigest()))
(audio/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Prepared eight short recorded character voices; no synthesized tones.')
