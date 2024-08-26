# pyrsistence - A Python extension for managing External Memory Data Structures (EMDs)

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


## Installing

**pyrsistence** can be directly compiled and installed using the following `pip`
command:

    pip install git+https://github.com/huku-/pyrsistence.git

Alternatively, you can add `git+https://github.com/huku-/pyrsistence.git` in your
project's **requirements.txt**.


## Quick start

Using **pyrsistence** is straightforward, as it exposes a minimal `list`-like and
`dict`-like API:

    import pyrsistence

    em_dict = pyrsistence.EMDict("/tmp/em_dict")
    em_dict["a"] = 1
    em_dict["b"] = 2
    print(list(em_dict.items()))
    em_dict.close()

    em_list = pyrsistence.EMList("/tmp/em_list")
    em_list.append(1)
    em_list.append(2)
    print(list(em_list))
    em_list.close()

Here are some real life examples:

* [Here](https://github.com/huku-/xde/blob/master/xde/em_shadow_memory.py) is a
  simple shadow memory implementation based on `EMList`

* [Here](https://github.com/huku-/xde/blob/master/xde/em_graph.py) simple graph
  implementation based on `EMDict`

* An `EMGraph` implementation based on `EMDict`, which integrates with NetworkX,
  can be found [here](examples/em_graph.py). A more advanced example, utilizing
  custom serialization and deserialization logic is [here](examples/em_graph_pickle.py).
  Have a look at the wiki for a deeper explanation.

For more information, have a look at the [wiki](https://github.com/huku-/pyrsistence/wiki).


## Contributors

Special thanks to the following people for contributing to **pyrsistence**:

  * [argp](https://github.com/argp), [vats](https://github.com/vats-) and
    [anestisb](https://github.com/anestisb) for using, testing, reporting and
    fixing bugs.

  * [adc](https://github.com/adc) for Python 3.x support.


## References

  * Halvar's [Code analysis and scale](https://lists.immunityinc.com/pipermail/dailydave/2015-September/000992.html)
  * [Algorithms and Data Structures for External Memory](https://www.ittc.ku.edu/~jsv/Papers/Vit.IO_book.pdf)
  * [Basic External Memory Data Structures](http://www.it-c.dk/people/pagh/papers/external.pdf)
  * [MMap: Fast Billion-Scale Graph Computation on a PC via Memory Mapping](http://www.cc.gatech.edu/~dchau/papers/14-bigdata-mmap.pdf)
  * [GraphChi: Large-Scale Graph Computation on Just a PC](http://select.cs.cmu.edu/publications/paperdir/osdi2012-kyrola-blelloch-guestrin.pdf)
  * [TurboGraph: A Fast Parallel Graph Engine Handling Billion-scale Graphs in a Single PC](http://www.eiti.uottawa.ca/~nat/Courses/csi5387_Winter2014/paper1.pdf)
  * [SSDAlloc: Hybrid SSD/RAM Memory Management Made Easy](https://www.usenix.org/legacy/events/nsdi11/tech/full_papers/Badam.pdf)
