#!/usr/bin/env python
'''em_dict_iter.py - Benchmark for external memory dictionary iterators.'''

__author__ = 'huku <huku@grhack.net>'


import sys
import shutil
import random
import time

import util
import pyrsistence


def main():

    # Initialize new external memory dictionary.
    util.msg('Populating external memory dictionary')

    t1 = time.time()

    dirname = util.make_temp_name('em_dict')

    em_dict = pyrsistence.EMDict(dirname)
    for i in util.xrange(random.randrange(0x1000)):
        k = random.randrange(0x100000)
        v = random.randrange(0x100000)
        em_dict[k] = v

    t2 = time.time()
    util.msg('Done in %d sec.' % (t2 - t1))


    # Request several iterator objects to locate possible memory leaks.
    util.msg('Testing item iterator')
    for i in util.xrange(0x1000):
        for item in em_dict.items():
            pass

    util.msg('Testing keys iterator')
    for i in util.xrange(0x1000):
        for key in em_dict.keys():
            pass

    util.msg('Testing values iterator')
    for i in util.xrange(0x1000):
        for value in em_dict.values():
            pass

    t3 = time.time()
    util.msg('Done in %d sec.' % (t3 - t2))


    # Close and remove external memory dictionary from disk.
    em_dict.close()
    shutil.rmtree(dirname)

    return 0


if __name__ == '__main__':
    sys.exit(main())

# EOF
