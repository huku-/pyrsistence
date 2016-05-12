#!/usr/bin/env python
'''util.py - Test suite utility functions.'''

__author__ = 'huku <huku@grhack.net>'

import os
import tempfile
import time

def msg(what):
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
    print '(%s) [*] %s' % (timestamp, what)

def make_temp_name(name):
    return '%s%s%s' % (tempfile.gettempdir(), os.path.sep, name)
