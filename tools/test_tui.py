#!/usr/bin/env python3
"""Exercise the actual TUI clients and relay in PTYs. Requires pyte.
Optional --screenshot PATH renders a captured terminal using record_showcase.py.
"""
import argparse
import codecs
import fcntl
import os
from pathlib import Path
import pty
import select
import socket
import struct
import subprocess
import termios
import time

import pyte

ROOT = Path(__file__).resolve().parents[1]


class Terminal:
    def __init__(self, port, course=None, profile=None):
        self.master, slave = pty.openpty()
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack('HHHH', 34, 110, 1100, 680))
        self.original = termios.tcgetattr(slave)
        self.slave = slave
        self.process = subprocess.Popen(
            [str(ROOT / 'terminal_velocity'), '--ssh-basic', '--mute', *(course or ['--summit']),
             '--connect', f'127.0.0.1:{port}', '--room', 'pty-test'],
            stdin=slave, stdout=slave, stderr=slave, start_new_session=True,
            env=dict(os.environ, **({'TV_PROGRESS_FILE': str(profile)} if profile else {})))
        self.screen = pyte.Screen(110, 34)
        self.stream = pyte.Stream(self.screen)
        self.decoder = codecs.getincrementaldecoder('utf-8')('replace')

    def read(self):
        if select.select([self.master], [], [], 0)[0]:
            self.stream.feed(self.decoder.decode(os.read(self.master, 131072)))
        assert self.process.poll() is None, 'TUI exited unexpectedly'

    def send(self, keys):
        os.write(self.master, keys.encode())

    def close(self):
        if self.process.poll() is None:
            self.send('q')
            try:
                self.process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.process.terminate()
                self.process.wait(timeout=3)
        restored = termios.tcgetattr(self.slave) == self.original
        os.close(self.master)
        os.close(self.slave)
        assert self.process.returncode == 0 and restored, 'exit or terminal restoration failed'


def pump(terminals, seconds):
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        for terminal in terminals:
            terminal.read()
        time.sleep(.005)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--screenshot')
    args = parser.parse_args()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
        probe.bind(('127.0.0.1', 0))
        port = probe.getsockname()[1]
    relay = subprocess.Popen([str(ROOT / 'terminal_velocity'), '--relay', str(port)],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    terminals = []
    try:
        assert select.select([relay.stdout], [], [], 3)[0], 'relay startup timed out'
        assert 'listening' in relay.stdout.readline(), 'relay failed to start'
        terminals = [Terminal(port), Terminal(port)]
        pump(terminals, .3)
        assert any('[3] SZCZYT' in row for row in terminals[0].screen.display)
        for terminal in terminals:
            terminal.send('\r')
        pump(terminals, .25)
        cells = [c for row in terminals[0].screen.buffer.values() for c in row.values()]
        assert all(c.data != '@' for c in cells), 'player marker still present'
        assert any(0x2800 <= ord(c.data) <= 0x28ff and c.fg == 'f4cd5e'
                   for c in cells if len(c.data) == 1), 'ready player is not yellow'
        terminals[1].send('d')
        pump(terminals, .35)
        terminals[1].send('s')
        pump(terminals, .15)
        for terminal in terminals:
            assert any('DUCHY 1' in row for row in terminal.screen.display), 'missing live peer'
        cells = [c for row in terminals[0].screen.buffer.values() for c in row.values()]
        assert any(0x2800 <= ord(c.data) <= 0x28ff and c.fg == '5dc9d2'
                   for c in cells if len(c.data) == 1), 'ghost body not rendered'
        if args.screenshot:
            from PIL import Image
            from record_showcase import paint
            image = Image.new('RGB', (1100, 680))
            paint(terminals[0].screen, image, {})
            image.save(args.screenshot)
        terminals[0].send(' ')
        pump(terminals, .10)
        terminals[0].send('x')
        pump(terminals, .05)
        cells = [c for row in terminals[0].screen.buffer.values() for c in row.values()]
        assert not any(0x2800 <= ord(c.data) <= 0x28ff and c.fg == 'f4cd5e'
                       for c in cells if len(c.data) == 1), 'spent dash still renders player yellow'
        print('PASS player loses yellow immediately after airborne dash')
        # Test actual menu input through the SSH parser, not just CLI selection.
        terminals[0].send('m2\r')
        pump(terminals, .2)
        assert any('EKSPEDYCJA' in row for row in terminals[0].screen.display)
        terminals[0].send('m3\r')
        pump(terminals, .2)
        assert any('SZCZYT' in row for row in terminals[0].screen.display)
        print('PASS two real TUI PTYs: menu selection, yellow body, no @, visible live ghost')
        for terminal in terminals:
            terminal.close()
        terminals = []
        print('PASS clean exits and terminal restoration in both PTYs')
        import tempfile
        with tempfile.TemporaryDirectory(prefix='tv-menu-') as folder:
            profile = Path(folder) / 'progress.dat'
            profile.write_text('TVP1 2\n10 0 0 2 1\n0 -1 0 0 0\n0 -1 0 0 0\n')
            campaign = Terminal(port, ['--chapter', '1'], profile)
            terminals = [campaign]
            pump(terminals, .3)
            assert any('[4] ZACISZE' in row for row in campaign.screen.display)
            campaign.send('5\r')
            pump(terminals, .2)
            assert any('WSPINACZKA / 2' in row for row in campaign.screen.display)
            campaign.send('m6')
            pump(terminals, .2)
            assert any('Ukończ poprzedni etap' in row for row in campaign.screen.display)
            campaign.send('4\r')
            pump(terminals, .2)
            assert any('WSPINACZKA / 1' in row for row in campaign.screen.display)
            campaign.close()
            terminals = []
        print('PASS campaign menu honors saved unlocks and blocks locked chapters')
    finally:
        for terminal in terminals:
            terminal.close()
        relay.terminate()
        relay.wait(timeout=3)


if __name__ == '__main__':
    main()
