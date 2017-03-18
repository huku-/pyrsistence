TESTS=em_dict_basic em_dict_check em_dict_iter \
	em_list_basic em_list_check em_list_iter
OBJS=util.o marshaller.o rbtree.o mapped_file.o em_dict.o em_list.o pyrsistence.o
BIN=pyrsistence.so

PYTHON_VERSION=

PYTHON=python$(PYTHON_VERSION)
PYTHON_INCLUDES=$(shell $(PYTHON)-config --includes)
PYTHON_LIBS=$(shell $(PYTHON)-config --ldflags)

# D=-D DEBUG -ggdb
D=-ggdb
CFLAGS+=-Wall -Wno-unused-result -Wno-strict-aliasing -O2 $(PYTHON_INCLUDES) \
	-fPIC $(D)
LDFLAGS+=$(PYTHON_LIBS) -shared

all: $(OBJS)
	$(CC) $(OBJS) -o $(BIN) $(LDFLAGS)

.PHONY: install
install:
	$(PYTHON) setup.py install

.PHONY: clean
clean:
	rm -fr $(OBJS) $(BIN) build
	find . -iname '*.pyc' -type f -exec rm -f \{\} \;
	find . -iname '__pycache__' -type d -exec rm -fr \{\} \;

.PHONY: test
test:
	for t in $(TESTS); do PYTHONPATH=. $(PYTHON) tests/$$t.py; done
