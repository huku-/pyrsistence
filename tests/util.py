#!/usr/bin/env python
'''util.py - Test suite utility functions.'''

__author__ = 'huku <huku@grhack.net>'


import sys
import os
import tempfile
import time


if sys.version_info >= (3, 0):
    xrange = range
else:
    xrange = xrange

def msg(what):
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
    sys.stdout.write('(%s) [*] %s\n' % (timestamp, what))

def make_temp_name(name):
    return '%s%s%s' % (tempfile.gettempdir(), os.path.sep, name)

