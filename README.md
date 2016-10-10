# pyrsistence - A Python extension for managing External Memory Data Structures (EMDs)

huku &lt;[huku@grhack.net](mailto:huku@grhack.net)&gt;

<a href="https://pledgie.com/campaigns/31289"><img alt="" src="https://pledgie.com/campaigns/31289.png?skin_name=chrome" border="0" ></a>


## About

**pyrsistence** is a WIP Python extension, developed in C, for managing
External Memory Data Structures (usually referred to as EMDs). At its current
version, **pyrsistence** supports external memory lists and dictionaries.

External Memory Data Structures are used to store data off-line (think of a
graph stored on the disk for example), while allowing for on-line access on
the data structure's elements (think of traversing the aforementioned graph
as if it's in the main memory). SQL/no-SQL servers are notable examples of
programs utilizing EMD related technologies, however **pyrsistence** is a
clean, minimal implementation for home use, that doesn't require any server
software (**pyrsistence** is, in fact, implemented purely using memory-mapped
files, a mature facility offered by all modern operating systems).

This extension was specially designed for big data analysis problems that may
arise during reverse engineering, however, it may be used for other purposes as
well. Halvar Flake describes this class of problems in his [DailyDave](https://lists.immunityinc.com/pipermail/dailydave/)
post entitled ["Code analysis and scale"](https://lists.immunityinc.com/pipermail/dailydave/2015-September/000992.html).

For more information on EMDs have a look at the references.


## Compiling pyrsistence

The code should compile without errors or warnings on all supported platforms.


### Microsoft Windows

Set **PYTHON_PREFIX** in **Makefile.nmake** to the directory where you have
installed Python and then run the following command to build a 32-bit version
of **pyrsistence**.

```
Z:\pyrsistence>"C:\Program Files (x86)\Microsoft Visual Studio 10.0\vc\vcvarsall.bat"
Z:\pyrsistence>nmake /F Makefile.nmake
```

Alternatively, run the following to produce a 64-bit version of **pyrsistence**.

```
Z:\pyrsistence>"C:\Program Files (x86)\Microsoft Visual Studio 10.0\vc\vcvarsall.bat" amd64
Z:\pyrsistence>nmake /F Makefile.nmake
```

In newer versions of Visual Studio you might need to replace the **amd64**
argument with **x86_amd64**.

To install **pyrsistence** issue the following command:

```
Z:\pyrsistence>nmake /F Makefile.nmake install
```

### MacOS X

Just enter the top-level directory and run **make** and **make install**.


### Linux

Just enter the top-level directory and run **make** and **make install**.


## Precompiled binaries

Below is a list of experimental, precompiled binaries for all supported platforms.

  * [pyrsistence-win-vs2012-python27.tgz](https://www.grhack.net/pyrsistence-win-vs2012-python27.tgz)
    (Microsoft Windows, Visual Studio 2012, Python 2.7)
  * [pyrsistence-macosx-llvm730-python27.tgz](https://www.grhack.net/pyrsistence-macosx-llvm730-python27.tgz)
    (MacOS X, LLVM 7.3.0, Python 2.7)
  * [pyrsistence-linux-gcc484-python27.tgz](https://www.grhack.net/pyrsistence-linux-gcc484-python27.tgz)
    (Linux, GCC 4.8.4, Python 2.7)


## Using pyrsistence

For more information on **pyrsistence**, have a look at the
[wiki](https://github.com/huku-/pyrsistence/wiki).


## References

  * Halvar's [Code analysis and scale](https://lists.immunityinc.com/pipermail/dailydave/2015-September/000992.html)
  * [Algorithms and Data Structures for External Memory](https://www.ittc.ku.edu/~jsv/Papers/Vit.IO_book.pdf)
  * [Basic External Memory Data Structures](http://www.it-c.dk/people/pagh/papers/external.pdf)
  * [MMap: Fast Billion-Scale Graph Computation on a PC via Memory Mapping](http://www.cc.gatech.edu/~dchau/papers/14-bigdata-mmap.pdf)
  * [GraphChi: Large-Scale Graph Computation on Just a PC](http://select.cs.cmu.edu/publications/paperdir/osdi2012-kyrola-blelloch-guestrin.pdf)
  * [TurboGraph: A Fast Parallel Graph Engine Handling Billion-scale Graphs in a Single PC](http://www.eiti.uottawa.ca/~nat/Courses/csi5387_Winter2014/paper1.pdf)
  * [SSDAlloc: Hybrid SSD/RAM Memory Management Made Easy](https://www.usenix.org/legacy/events/nsdi11/tech/full_papers/Badam.pdf)

