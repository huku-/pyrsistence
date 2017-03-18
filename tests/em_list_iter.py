#!/usr/bin/env python
'''em_list_iter.py - Benchmark for external memory list iterators.'''

__author__ = 'huku <huku@grhack.net>'


import sys
import shutil
import random
import time

import util
import pyrsistence


def main(argv):

    # Initialize new external memory list.
    util.msg('Populating external memory list')

    t1 = time.time()

    dirname = util.make_temp_name('em_list')

    em_list = pyrsistence.EMList(dirname)
    for i in util.xrange(0x1000):
        em_list.append(random.randrange(0x100000))

    t2 = time.time()
    util.msg('Done in %d sec.' % (t2 - t1))


    # Request several iterator objects to locate possible memory leaks.
    util.msg('Testing iterator')
    for i in util.xrange(0x1000):
        for item in em_list:
            pass

    t3 = time.time()
    util.msg('Done in %d sec.' % (t3 - t2))

    # Close and remove external memory list from disk.
    em_list.close()
    shutil.rmtree(dirname)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

# EOF
