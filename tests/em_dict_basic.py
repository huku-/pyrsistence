#!/usr/bin/env python
'''em_dict_basic.py - Basic benchmark for external memory dictionary.'''

__author__ = 'huku <huku@grhack.net>'


import sys
import shutil
import random
import time

import util
import pyrsistence


def main(argv):

    # Initialize new external memory dictionary.
    util.msg('Populating external memory dictionary')

    t1 = time.time()

    dirname = util.make_temp_name('em_dict')

    em_dict = pyrsistence.EMDict(dirname)
    for i in xrange(0x1000000):
        em_dict[i] = i

    t2 = time.time()
    util.msg('Done in %d sec.' % (t2 - t1))

    # Close and remove external memory dictionary from disk.
    em_dict.close()
    shutil.rmtree(dirname)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

# EOF
