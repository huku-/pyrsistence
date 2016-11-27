TESTS=em_dict_basic em_dict_check em_dict_iter \
	em_list_basic em_list_check em_list_iter
OBJS=util.o marshaller.o rbtree.o mapped_file.o em_dict.o em_list.o pyrsistence.o
BIN=pyrsistence.so

PYTHON_INCLUDES=$(shell python2.7-config --includes)
PYTHON_LIBS=$(shell python2.7-config --ldflags)

# D=-D DEBUG -ggdb
D=
CFLAGS += -Wall -Wno-unused-result -Wno-strict-aliasing -O2 $(PYTHON_INCLUDES) \
	-fPIC $(D)
LDFLAGS += $(PYTHON_LIBS) -shared

all: $(OBJS)
	$(CC) $(OBJS) -o $(BIN) $(LDFLAGS)

.PHONY: install
install:
	python setup.py install

.PHONY: clean
clean:
	rm -fr $(OBJS) $(BIN) build

.PHONY: test
test:
	for t in $(TESTS); do PYTHONPATH=. python tests/$$t.py; done
