#!/usr/bin/env python3
"""Real PTY flow: shared Unicode UI, dialogue, language, resize, save, restore."""
import fcntl
import os
from pathlib import Path
import struct
import tempfile
import termios
import pyte
from test_tui import Terminal, pump

def screen(t):
    return '\n'.join(t.screen.display)

def expect(t, text):
    pump([t], .16)
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
        t.send('d');pump([t],.30);t.send('s');pump([t],.05)
        t.send('e');expect(t,'HELENA')
        assert 'You' in screen(t),screen(t)
        for _ in range(6):t.send('e');pump([t],.10)
        t.send('\t');expect(t,"COURIER'S JOURNAL")
        t.send('\t');pump([t],.10)
        t.send('j');expect(t,'DISTRICTS OF THE ICEBERG')
        t.send('j');pump([t],.10)
        t.send('m');expect(t,'CONTINUE JOURNEY')
        t.send('5\r');expect(t,'Music:')
        t.send('1');pump([t],.15)
        t.send('\r');pump([t],.15)
        resize(t,76,24);expect(t,'CONTINUE JOURNEY')
        # Resize below supported size suspends gameplay and gives a readable message.
        resize(t,60,20)
        assert '76' in screen(t) or 'terminal' in screen(t).lower(),screen(t)
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
print('PASS actual PTY: bilingual menu, dialogue, journal/map, settings, resize, endless, continue, terminal restoration')
