#!/usr/bin/env python3
import os,signal,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
server=subprocess.Popen([sys.executable,str(root/'multiplayer/server.py'),'--engine',str(root/'multiplayer_engine')],cwd=root,
 stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,start_new_session=True)
try:
 line=server.stdout.readline()
 if line.strip()!='Multiplayer ready':raise RuntimeError('Server failed to start: '+server.stderr.read())
 subprocess.run([sys.executable,str(root/'multiplayer/test_service.py')],cwd=root,check=True,timeout=90)
finally:
 os.killpg(server.pid,signal.SIGTERM)
 try:server.wait(timeout=5)
 except subprocess.TimeoutExpired:os.killpg(server.pid,signal.SIGKILL);server.wait()
