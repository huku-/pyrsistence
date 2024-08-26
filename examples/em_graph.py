from __future__ import annotations

from typing import Any

import os
import pathlib
import tempfile
import uuid

import networkx

import pyrsistence


class ObservedDict(dict):

    def __new__(cls, *args: Any, **kwargs: Any) -> ObservedDict:
        self = super().__new__(cls, *args, **kwargs)
        self._observer = None
        return self

    def __setitem__(self, key: Any, value: Any) -> None:
        super().__setitem__(key, value)
        if self._observer:
            observer, key = self._observer
            observer[key] = self

    def __getstate__(self) -> dict:
        return dict(self.__dict__, _observer=None)


class EMDict(pyrsistence.EMDict):

    def __init__(self, path: Path | str = None) -> None:
        self._path = (
            path or pathlib.Path(tempfile.gettempdir()) / f"em_dict-{uuid.uuid1()}"
        )
        super().__init__(str(self._path))
        print(f"EMDict created at {self._path}")

    def __setitem__(self, key: Any, value: Any) -> None:
        if isinstance(value, ObservedDict):
            value._observer = (self, key)
        super().__setitem__(key, value)

    def __getitem__(self, key: Any) -> Any:
        value = super().__getitem__(key)
        if isinstance(value, ObservedDict):
            value._observer = (self, key)
        return value

    def __getstate__(self) -> dict:
        return {"path": self._path}

    def __setstate__(self, state: dict) -> None:
        path = state["path"]
        self.__init__(path)


class Graph(networkx.DiGraph):
    node_dict_factory = ObservedDict
    node_attr_dict_factory = ObservedDict
    adjlist_outer_dict_factory = EMDict
    adjlist_inner_dict_factory = ObservedDict
    edge_attr_dict_factory = ObservedDict
    graph_attr_dict_factory = ObservedDict


def main() -> None:

    em_graph = Graph()

    while True:
        random_graph = networkx.erdos_renyi_graph(20, 0.1, directed=True)
        if networkx.is_weakly_connected(random_graph):
            break

    em_graph.add_nodes_from(random_graph)
    em_graph.add_edges_from(random_graph.edges())

    print(f"Random graph : {random_graph}")
    print(f"EM graph     : {em_graph}")

    #
    # Now em_graph and random_graph should be equal. Run a simple test over the
    # dominating sets.
    #

    random_doms = networkx.immediate_dominators(random_graph, 0)
    em_doms = networkx.immediate_dominators(em_graph, 0)

    assert random_doms == em_doms, "Immediate dominators not the same"

    print("Test successful")


if __name__ == "__main__":
    main()
