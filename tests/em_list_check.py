#!/usr/bin/env python
'''em_list_check.py - Integrity benchmark for external memory list.'''

__author__ = 'huku <huku@grhack.net>'


import sys
import shutil
import random
import time

import util
import pyrsistence


def main(argv):

    # Initialize new external memory list.
    util.msg('Populating normal and external memory list')

    t1 = time.time()

    dirname = util.make_temp_name('em_list')

    l = []
    em_list = pyrsistence.EMList(dirname)
    for i in xrange(0x1000000):
        v = random.randrange(0x1000000)
        em_list.append(v)
        l.append(v)

    t2 = time.time()
    util.msg('Done in %d sec.' % (t2 - t1))

    util.msg('Verifying external memory list contents')

    for i in xrange(0x1000000):
        if em_list[i] != l[i]:
            util.msg('FATAL! Mismatch in element %d: Got %#x but expected %#x' % (i, em_list[i], l[i]))

    t3 = time.time()
    util.msg('Done in %d sec.' % (t3 - t2))

    # Close and remove external memory list from disk.
    em_list.close()
    shutil.rmtree(dirname)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

# EOF
