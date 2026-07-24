CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread -fPIC -I./include
SRC     = src/heap.c src/freelist.c src/thread_cache.c src/lethe.c

.PHONY: all test bench clean

all: libLethe.so

libLethe.so: $(SRC)
	$(CC) $(CFLAGS) -shared -o $@ $^

test: libLethe.so
	$(CC) $(CFLAGS) tests/test_basic.c -L. -lLethe -o test_basic && LD_LIBRARY_PATH=. ./test_basic
	$(CC) $(CFLAGS) tests/test_coalesce.c -L. -lLethe -o test_coalesce && LD_LIBRARY_PATH=. ./test_coalesce
	$(CC) $(CFLAGS) tests/test_threads.c -L. -lLethe -o test_threads && LD_LIBRARY_PATH=. ./test_threads

bench: libLethe.so
	$(CC) $(CFLAGS) bench/bench.c -L. -lLethe -o bench_lethe
	$(CC) $(CFLAGS) bench/bench.c -o bench_glibc
	@echo "\n=== Lethe ===" && LD_LIBRARY_PATH=. ./bench_lethe
	@echo "\n=== glibc ===" && ./bench_glibc

clean:
	rm -f libLethe.so test_basic test_coalesce test_threads bench_lethe bench_glibc
