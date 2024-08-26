#!/usr/bin/env python
#
# When modifying this file, always keep in mind that it has to work on both
# Python 2.x and 3.x.
#

import functools
import os
import platform
import setuptools
import sys


@functools.cache
def get_dir():
    return os.path.dirname(os.path.abspath(__file__))


def get_cflags():
    cflags = []
    system = platform.system().lower()
    if system == "linux":
        cflags += [
            "-ggdb",
            "-Wall",
            "-Wextra",
            "-O2",
            "-fPIC",
            "-D_GNU_SOURCE",
            "-D_FILE_OFFSET_BITS=64",
            # "-DDEBUG"
        ]
    elif system == "windows":
        cflags += [
            "/nologo",
            "/Zi",
            "/W3",
            "/O2",
            # "/D DEBUG"
        ]
    return cflags


def get_sources():
    return [
        name
        for name in os.listdir(get_dir())
        if os.path.isfile(os.path.join(get_dir(), name)) and name.endswith(".c")
    ]


def main(argv):

    pyrsistence_mod = setuptools.Extension(
        "pyrsistence",
        extra_compile_args=get_cflags(),
        sources=[os.path.join(get_dir(), name) for name in get_sources()],
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
