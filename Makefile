pagesize := $(or $(shell getconf PAGESIZE),4096)
cppflags := -Iinclude -pthread -DPAGESIZE=${pagesize} $(CPPFLAGS)

HEADERS = \
  include/bev/linear_ringbuffer.hpp \
  include/bev/io_buffer.hpp

all: benchmark tests

benchmark: benchmark.cpp $(HEADERS) Makefile
	g++ $< -O2 -g3 -o $@ $(cppflags) $(CFLAGS) $(CXXFLAGS)

tests: tests.cpp $(HEADERS) Makefile
	g++ $< -g3 -o $@ $(cppflags) $(CFLAGS) $(CXXFLAGS)


PREFIX ?= /usr/local
install:
	install -d $(DESTDIR)$(PREFIX)
	cp -R include/ $(DESTDIR)$(PREFIX)
