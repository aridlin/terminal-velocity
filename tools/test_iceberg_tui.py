#!/usr/bin/env python3
"""Real PTY flow: shared Unicode UI, dialogue, language, resize, save, restore."""
import fcntl
import os
from pathlib import Path
import struct
import tempfile
import termios
import time
import pyte
from test_tui import Terminal, pump

def screen(t):
    return '\n'.join(t.screen.display)

def expect(t, text):
    deadline=time.monotonic()+2
    pump([t], .10)
    while text not in screen(t) and time.monotonic()<deadline:pump([t], .03)
    assert text in screen(t), f'Missing {text!r}:\n{screen(t)}'

def resize(t, cols, rows):
    fcntl.ioctl(t.slave, termios.TIOCSWINSZ, struct.pack('HHHH', rows, cols, cols*10, rows*20))
    t.screen.resize(rows, cols)
    pump([t], .25)

with tempfile.TemporaryDirectory(prefix='tv-iceberg-pty-') as folder:
    profile=Path(folder)/'progress.dat'
    t=Terminal(None,['--campaign','--seed','73'],profile)
    try:
        expect(t,'ROZPOCZNIJ PODRÓŻ')
        t.send('l');expect(t,'BEGIN JOURNEY')
        t.send('\r');expect(t,'LOWER HARBOR')
        t.send('i');deadline=time.monotonic()+.6
        def dash_spent():return any(c.fg=='83c5ce' for row in t.screen.buffer.values() for c in row.values())
        while not dash_spent() and time.monotonic()<deadline:pump([t],.02)
        assert dash_spent(),'Basic SSH upward dash did not spend the dash\n'+screen(t)
        t.send('r');expect(t,'Retries 1')
        t.send('d');deadline=time.monotonic()+2
        while '[E] Talk' not in screen(t) and time.monotonic()<deadline:pump([t],.01)
        pump([t],.12)  # Walk inside the interaction area before braking.
        t.send('s');pump([t],.08)
        assert '[E] Talk' in screen(t), 'Never reached the dispatcher\n'+screen(t)
        t.send('b');expect(t,'INSPECT')
        assert 'HELENA' in screen(t), screen(t)
        t.send('b');pump([t],.10)
        t.send('e');expect(t,'HELENA')
        expect(t,'You')
        for _ in range(20):
            if 'E/ENTER' not in screen(t):break
            t.send('e');pump([t],.10)
        t.send('\t');expect(t,"QUEST BOOK")
        t.send('\t');pump([t],.10)
        t.send('j');expect(t,'JOURNEY MAP')
        t.send('w');expect(t,'▸ □ LAZARUS HEATING WORKS')
        t.send('\r');expect(t,'Closed.')
        assert 'JOURNEY MAP' in screen(t),screen(t)
        t.send('b');expect(t,'[Route]')
        t.send('j');pump([t],.10)
        t.send('m');expect(t,'CONTINUE JOURNEY')
        t.send('5\r');expect(t,'Music:')
        t.send('1');pump([t],.15)
        t.send('\r');pump([t],.15)
        resize(t,76,24);expect(t,'CONTINUE JOURNEY')
        # Compact layout remains usable; unsupported sizes explain the minimum.
        resize(t,60,20);expect(t,'Continue journey')
        resize(t,32,19)
        assert '34' in screen(t) or 'Resize' in screen(t),screen(t)
        resize(t,110,34);expect(t,'CONTINUE JOURNEY')
        t.send('3\r');expect(t,'ENDLESS')
        t.send('m');expect(t,'RESUME ENDLESS EXPEDITION')
    finally:t.close()
    assert Path(str(profile)+'.iceberg').exists(),'No campaign save'
    t=Terminal(None,['--campaign','--seed','73'],profile)
    try:
        expect(t,'CONTINUE JOURNEY')
        t.send('\r');expect(t,'LOWER HARBOR')
        t.send('p');expect(t,'A MOMENT TO BREATHE')
    finally:t.close()
print('PASS actual PTY: bilingual menu, basic vertical dash, dialogue, journal/map, settings, resize, endless, continue, terminal restoration')
