#!/usr/bin/env python
'''em_dict_check.py - Integrity benchmark for external memory dictionary.'''

__author__ = 'huku <huku@grhack.net>'


import sys
import shutil
import random
import time

import util
import pyrsistence


def main(argv):

    # Initialize new external memory dictionary.
    util.msg('Populating normal and external memory dictionary')

    t1 = time.time()

    dirname = util.make_temp_name('em_dict')

    d = {}
    em_dict = pyrsistence.EMDict(dirname)
    for i in util.xrange(0x1000000):
        v = random.randrange(0x1000000)
        em_dict[i] = v
        d[i] = v

    t2 = time.time()
    util.msg('Done in %d sec.' % (t2 - t1))

    util.msg('Verifying external memory dictionary contents')

    for i in util.xrange(0x1000000):
        if em_dict[i] != d[i]:
            util.msg('FATAL! Mismatch in element %d: Got %#x but expected %#x' % (i, em_dict[i], d[i]))

    t3 = time.time()
    util.msg('Done in %d sec.' % (t3 - t2))

    # Close and remove external memory dictionary from disk.
    em_dict.close()
    shutil.rmtree(dirname)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

# EOF
