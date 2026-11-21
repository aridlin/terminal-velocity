#!/usr/bin/env python3
"""Real socket tests: shared private lobby, native/browser transport, validation, reconnect."""
import asyncio,struct,sys
from websockets.asyncio.client import connect

async def frame(ws):
 for _ in range(40):
  data=await asyncio.wait_for(ws.recv(),10)
  if isinstance(data,bytes):
   cols,rows=struct.unpack_from('<II',data)
   if rows&0x80000000:
    rows&=0x7fffffff;expanded=bytearray(struct.pack('<II',cols,rows));at=8
    while at<len(data):
     run=struct.unpack_from('<H',data,at)[0];at+=2;expanded.extend(data[at:at+16]*run);at+=16
    data=expanded
   assert 34<=cols<=160 and (24 if cols<56 else 20)<=rows<=60 and len(data)==8+cols*rows*16
   text=''.join(chr(struct.unpack_from('<I',data,8+i*16)[0]) for i in range(cols*rows))
   return text
 raise AssertionError('No frame')
async def main():
 uri='ws://127.0.0.1:27889'
 for mode in ['race','coop']:
  async with connect(uri,origin='http://127.0.0.1:8877') as one:
   await one.send(f'JOIN new {mode} 1 -')
   welcome=(await one.recv()).split();assert welcome[0]=='WELCOME';code=welcome[1];token=welcome[3]
   reader,writer=await asyncio.open_connection('127.0.0.1',27890)
   writer.write(f'JOIN {code} {mode} 1 -\n'.encode());await writer.drain()
   n=struct.unpack('<I',await reader.readexactly(4))[0]
   assert (await reader.readexactly(n)).startswith(b'WELCOME ')
   async with connect(uri,origin='http://127.0.0.1:8877') as third:
    await third.send(f'JOIN {code} {mode} 1 -');assert (await third.recv()).startswith('ERROR ')
   await one.send('I 256 76 34 1');writer.write(b'I 256 76 34 1\n');await writer.drain()
   text=await frame(one);assert code in text
   # Malformed native input must disconnect only the offender, preserving the match.
   writer.write(b'I -1 76 34 1\n');await writer.drain()
   # Read buffered frame(s) followed by error.
   for _ in range(30):
    n=struct.unpack('<I',await reader.readexactly(4))[0];data=await reader.readexactly(n)
    if data.startswith(b'ERROR '):break
   else:raise AssertionError('Bad input was accepted')
   writer.close();await writer.wait_closed()
   print('PASS',mode,'shared browser/native lobby, frame dimensions, full-room rejection, invalid-input rejection')
  await asyncio.sleep(.1)
  async with connect(uri,origin='http://127.0.0.1:8877') as resumed:
   await resumed.send(f'JOIN {code} {mode} 1 {token}');assert (await resumed.recv()).startswith('WELCOME ')
   await resumed.send('I 0 34 24 0');await frame(resumed)
   print('PASS reconnect identity and compact Polish frame')
 async with connect(uri,origin='http://127.0.0.1:8877') as bad:
  await bad.send('JOIN new race 999 -');assert (await bad.recv()).startswith('ERROR ')
 print('PASS incompatible protocol rejected')
if __name__=='__main__':asyncio.run(main())
