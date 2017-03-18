#!/usr/bin/env python
'''em_list_basic.py - Basic benchmark for external memory list.'''

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
    for i in util.xrange(0x1000000):
        em_list.append(i)

    t2 = time.time()
    util.msg('Done in %d sec.' % (t2 - t1))

    # Close and remove external memory list from disk.
    em_list.close()
    shutil.rmtree(dirname)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

# EOF
