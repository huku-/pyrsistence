#!/usr/bin/env python
#
# When modifying this file, always keep in mind that it has to work on both
# Python 2.x and 3.x.
#

import os
import setuptools
import sys


DIR = os.path.dirname(os.path.abspath(__file__))

CFLAGS = [
    "-ggdb",
    "-Wall",
    "-Wextra",
    "-Wno-unused-result",
    "-Wno-unused-parameter",
    "-Wno-strict-aliasing",
    "-Wno-cast-function-type",
    "-O2",
    "-fPIC",
    # "-DDEBUG"
]


def get_sources():
    return [
        name
        for name in os.listdir(DIR)
        if os.path.isfile(os.path.join(DIR, name)) and name.endswith(".c")
    ]


def main(argv):

    pyrsistence_mod = setuptools.Extension(
        "pyrsistence",
        extra_compile_args=CFLAGS,
        sources=[os.path.join(DIR, name) for name in get_sources()],
    )

    setuptools.setup(
        name="pyrsistence",
        version="1.1",
        description="pyrsistence",
        author="huku",
        author_email="huku@grhack.net",
        url="https://github.com/huku-/pyrsistence",
        ext_modules=[pyrsistence_mod],
    )

    return os.EX_OK


if __name__ == "__main__":
    sys.exit(main(sys.argv))
