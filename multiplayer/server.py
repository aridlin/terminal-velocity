#!/usr/bin/env python3
"""Private two-player matches; one authoritative, save-isolated game process per room.
WebSocket :27889 and native length-prefixed TCP :27890. Put WSS behind TLS.
"""
import asyncio, argparse, secrets, time, contextlib, struct, re, signal
from pathlib import Path
from websockets.asyncio.server import serve

def pack_frame(data):
    cols,rows=struct.unpack_from('<II',data)
    result=bytearray(struct.pack('<II',cols,rows|0x80000000))
    at=8
    while at<len(data):
        tile=data[at:at+16];run=1
        while run<65535 and at+(run+1)*16<=len(data) and data[at+run*16:at+(run+1)*16]==tile:run+=1
        result.extend(struct.pack('<H',run));result.extend(tile);at+=run*16
    return bytes(result) if len(result)<len(data) else data

class Room:
    def __init__(self, owner, code, mode, process):
        self.owner, self.code, self.mode, self.process = owner, code, mode, process
        self.peers = [None, None]
        self.tokens = [secrets.token_hex(16), secrets.token_hex(16)]
        self.empty_since = None
        self.jobs = [asyncio.create_task(self.tick()), asyncio.create_task(self.read())]
    def command(self, line):
        if self.process.returncode is None:
            self.process.stdin.write((line+'\n').encode())
    async def tick(self):
        try:
            deadline = asyncio.get_running_loop().time()
            while self.process.returncode is None:
                now = asyncio.get_running_loop().time()
                if not any(self.peers):
                    if self.empty_since is None:self.empty_since=now
                    if now-self.empty_since>60:break
                else:self.empty_since=None
                if now-deadline>.25:deadline=now # bounded catch-up, no time jump
                self.command('T');await self.process.stdin.drain()
                deadline+=1/120
                await asyncio.sleep(max(0,deadline-asyncio.get_running_loop().time()))
        finally:await self.close()
    async def read(self):
        try:
            while line:=await self.process.stdout.readline():
                slot, payload=line.split(b' ',1);peer=self.peers[int(slot)]
                if peer:await asyncio.wait_for(peer.send(pack_frame(bytes.fromhex(payload.decode()))),2)
        except (ValueError,ConnectionError,asyncio.TimeoutError):pass
        finally:await self.close()
    async def close(self):
        if self.owner.rooms.get(self.code) is not self:return
        self.owner.rooms.pop(self.code,None)
        for job in self.jobs:
            if job is not asyncio.current_task():job.cancel()
        for peer in self.peers:
            if peer:
                with contextlib.suppress(Exception):await peer.close()
        if self.process.returncode is None:
            self.process.terminate()
            with contextlib.suppress(asyncio.TimeoutError):await asyncio.wait_for(self.process.wait(),3)
            if self.process.returncode is None:self.process.kill()

class Service:
    def __init__(self,engine,max_rooms=16):self.engine,self.max_rooms,self.rooms=engine,max_rooms,{}
    async def session(self,peer):
        room=None;slot=None
        try:
            hello=await asyncio.wait_for(peer.recv(),10)
            fields=hello.split()
            if len(fields)!=5 or fields[0]!='JOIN':raise ValueError('JOIN code race|coop version token')
            _,code,mode,version,token=fields
            if version!='1' or mode not in ('race','coop'):raise ValueError('Version or mode mismatch')
            if code=='new':
                if len(self.rooms)>=self.max_rooms:raise ValueError('Server full')
                while True:
                    code=str(secrets.randbelow(900000)+100000)
                    if code not in self.rooms:break
                proc=await asyncio.create_subprocess_exec(self.engine,mode,str(secrets.randbits(31)),code,
                    stdin=asyncio.subprocess.PIPE,stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.DEVNULL,limit=400000)
                room=Room(self,code,mode,proc);self.rooms[code]=room
            else:
                if not re.fullmatch(r'[0-9]{6}',code):raise ValueError('Six-digit room code required')
                room=self.rooms.get(code)
                if not room or room.mode!=mode:raise ValueError('Room unavailable or different mode')
            if token!='-':
                if token not in room.tokens:raise ValueError('Reconnect rejected')
                slot=room.tokens.index(token)
                if room.peers[slot]:raise ValueError('Courier already connected')
            else:
                slot=next((i for i,p in enumerate(room.peers) if p is None),None)
                if slot is None:raise ValueError('Room full')
                # An occupied, disconnected identity needs its token to resume.
                if getattr(room,'joined',set()) and slot in room.joined:raise ValueError('Use reconnect token')
            room.joined=getattr(room,'joined',set())|{slot}
            room.peers[slot]=peer;room.command(f'C {slot} 1')
            await peer.send(f'WELCOME {code} {slot} {room.tokens[slot]}')
            interval=time.monotonic();count=0
            while True:
                line=await asyncio.wait_for(peer.recv(),10)
                if len(line)>100:raise ValueError('Input too long')
                f=line.split()
                if len(f)!=5 or f[0]!='I':raise ValueError('Invalid input')
                mask,cols,rows,language=map(int,f[1:])
                if not (0<=mask<1<<18 and 34<=cols<=160 and 24<=rows<=60 and language in (0,1)):raise ValueError('Invalid input')
                now=time.monotonic()
                if now-interval>=1:interval=now;count=0
                count+=1
                if count>100:raise ValueError('Input rate exceeded')
                room.command(f'I {slot} {mask} {cols} {rows} {language}')
        except (ValueError,asyncio.TimeoutError,ConnectionError,EOFError) as error:
            with contextlib.suppress(Exception):await peer.send('ERROR '+str(error))
        finally:
            if room and slot is not None and room.peers[slot] is peer:
                room.peers[slot]=None;room.command(f'C {slot} 0')
            with contextlib.suppress(Exception):await peer.close()

class TCPPeer:
    def __init__(self,reader,writer):self.reader,self.writer=reader,writer
    async def recv(self):
        line=await self.reader.readline()
        if not line:raise EOFError()
        return line.decode('ascii').strip()
    async def send(self,data):
        if isinstance(data,str):data=data.encode()
        self.writer.write(struct.pack('<I',len(data))+data);await self.writer.drain()
    async def close(self):self.writer.close();await self.writer.wait_closed()

async def main(args):
    service=Service(str(Path(args.engine).resolve()),args.max_rooms)
    async def tcp(reader,writer):await service.session(TCPPeer(reader,writer))
    async def websocket(ws):
        try:await service.session(ws)
        except Exception:await ws.close()
    native=await asyncio.start_server(tcp,args.tcp_host or args.host,args.tcp_port,limit=256)
    async with native,serve(websocket,args.host,args.ws_port,max_size=256,max_queue=4,
        origins=args.origin,compression='deflate',ping_interval=15,ping_timeout=15):
        print('Multiplayer ready',flush=True)
        stop=asyncio.Event()
        asyncio.get_running_loop().add_signal_handler(signal.SIGTERM,stop.set)
        await stop.wait()
        await asyncio.gather(*(room.close() for room in list(service.rooms.values())),return_exceptions=True)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--engine',default='./multiplayer_engine');p.add_argument('--host',default='127.0.0.1')
    p.add_argument('--ws-port',type=int,default=27889);p.add_argument('--tcp-port',type=int,default=27890);p.add_argument('--max-rooms',type=int,default=16);p.add_argument('--tcp-host')
    p.add_argument('--origin',action='append',default=['https://aridlin.pl','https://prol.aridlin.pl','http://127.0.0.1:8877'])
    asyncio.run(main(p.parse_args()))
