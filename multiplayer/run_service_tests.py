#!/usr/bin/env python3
import os,signal,subprocess,sys,socket,select
from pathlib import Path
root=Path(__file__).resolve().parents[1]
def free_port():
 with socket.socket() as sock:sock.bind(('127.0.0.1',0));return sock.getsockname()[1]
ws_port,tcp_port=free_port(),free_port()
server=subprocess.Popen([sys.executable,str(root/'multiplayer/server.py'),'--engine',str(root/'multiplayer_engine'),'--ws-port',str(ws_port),'--tcp-port',str(tcp_port)],cwd=root,
 stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,start_new_session=True)
try:
 if not select.select([server.stdout],[],[],15)[0]:raise RuntimeError('Server startup timed out')
 line=server.stdout.readline()
 if line.strip()!='Multiplayer ready':raise RuntimeError('Server failed to start: '+server.stderr.read())
 subprocess.run([sys.executable,str(root/'multiplayer/test_service.py')],cwd=root,check=True,timeout=90,env=dict(os.environ,TV_MP_TEST_WS_PORT=str(ws_port),TV_MP_TEST_TCP_PORT=str(tcp_port)))
finally:
 os.killpg(server.pid,signal.SIGTERM)
 try:server.wait(timeout=5)
 except subprocess.TimeoutExpired:os.killpg(server.pid,signal.SIGKILL);server.wait()
