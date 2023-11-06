VERSION = 0.1.4

MAKEFLAGS += --silent

.PHONY: target-dir ./target/libsonic.a ./target/camel.a clean

all: build

build: ./target/libsonic.a
build-sanitized: ./target/libsonic-sanitized.a

OBJS = ./target/sonic.o ./target/utils.o ./target/server.o ./target/client.o ./target/rate_limiter.o

ifdef OPENSSL_PREFIX
	OPENSSL_CFLAGS=-I$(OPENSSL_PREFIX)/include
	OPENSSL_LDFLAGS=-L$(OPENSSL_PREFIX)/lib
	# Also export OPENSSL_PREFIX so it ends up in deps sub-Makefiles
	export OPENSSL_PREFIX
endif

CFLAGS += -g -std=c11 $(OPENSSL_CFLAGS)
LDFLAGS += -lssl -lcrypto $(OPENSSL_LDFLAGS)

WARN_CFLAGS += -Werror -Wall -Wextra -Wformat -Wformat-security -Warray-bounds

ifeq ($(shell uname), Linux)
CFLAGS_SANITIZED += -fsanitize=address,undefined,leak
LDFLAGS_SANITIZED += -lasan -lubsan 
endif
CFLAGS_SANITIZED +=
LDFLAGS_SANITIZED += -lssl -lcrypto

LIBSONIC = ./target/libsonic.a
LIBSONIC_SANITIZED = ./target/libsonic-sanitized.a

ifndef CC
CC = gcc
endif
LD = gcc

setup:

camel: ./target/camel.a

target-dir:
	mkdir -p target	
	mkdir -p target/examples
	mkdir -p target/examples/client && mkdir -p target/examples/server

$(LIBSONIC): target-dir camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c ./src/sonic.c -o ./target/sonic.o
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c ./src/utils.c -o ./target/utils.o
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c ./src/server/server.c -o ./target/server.o
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c ./src/server/rate_limiter.c -o ./target/rate_limiter.o
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c ./src/client/client.c -o ./target/client.o
	ar rcs $(LIBSONIC) $(OBJS)

$(LIBSONIC_SANITIZED): target-dir camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZED) -c ./src/sonic.c -o ./target/sonic.o
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZED) -c ./src/utils.c -o ./target/utils.o
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZED) -c ./src/server/server.c -o ./target/server.o
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZED) -c ./src/server/rate_limiter.c -o ./target/rate_limiter.o
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZED) -c ./src/client/client.c -o ./target/client.o
	ar rcs $(LIBSONIC_SANITIZED) $(OBJS)


# EXAMPLES ========================================================================================
run-client-example: build-sanitized 
	$(CC) -g $(CFLAGS_SANITIZED) ./examples/client/$(E).c $(LIBSONIC_SANITIZED) $(LDFLAGS_SANITIZED) -o ./target/examples/client/$(E)
	./target/examples/client/$(E)

run-server-example: build
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS) -c ./examples/server/$(E).c
	mv ./*.o ./target/
	
	$(LD) ./target/$(E).o $(LIBSONIC) $(LDFLAGS) -o ./target/examples/server/$(E)
	
	./target/examples/server/$(E)

#========================================================================================


# TESTS ========================================================================================
test: build-tests run-tests

build-tests: build-unit-tests build-integration-tests build-functional-tests build-fuzz-tests
run-tests: run-unit-tests run-integration-tests run-fuzz-tests run-functional-tests

build-unit-tests:
	cd tests/server/ && $(MAKE) build-unit-tests
	cd tests/client/ && $(MAKE) build-unit-tests

run-unit-tests:
	cd tests/server/ && $(MAKE) run-unit-tests
	cd tests/client/ && $(MAKE) run-unit-tests

build-integration-tests:
	cd tests/server/ && $(MAKE) build-integration-tests
	cd tests/client/ && $(MAKE) build-integration-tests

run-integration-tests:
	cd tests/server/ && $(MAKE) run-integration-tests
	cd tests/client/ && $(MAKE) run-integration-tests

build-functional-tests:
	cd tests/server/ && $(MAKE) build-functional-tests
	cd tests/client/ && $(MAKE) build-functional-tests

run-functional-tests:
	cd tests/server/ && $(MAKE) run-functional-tests
	cd tests/client/ && $(MAKE) run-functional-tests

build-fuzz-tests:
	cd tests/server/ && $(MAKE) build-fuzz-tests
	cd tests/client/ && $(MAKE) build-fuzz-tests

run-fuzz-tests:
	cd tests/server/ && $(MAKE) run-fuzz-tests
	cd tests/client/ && $(MAKE) run-fuzz-tests

#========================================================================================


# LIBS ========================================================================================
sync: sync-camel sync-libs-headers

sync-libs-headers:
	cp ./lib/camel/camel.h ./include/camel.h

sync-camel:
	rm -rf ./lib/camel/
	rsync -av --exclude='target' --exclude='.git' ../camel/ ./lib/camel/

./target/camel.a:
	cd ./lib/camel/ && $(MAKE)
	cp ./lib/camel/target/libcamel.a ./target/camel.a

#========================================================================================


clean:
	rm -f -r ./target/*

downstream:
	git fetch && git pull
	
upstream:
	git add .
	@read -p "Enter commit message: " message; \
	git commit -m "$$message"
	git push

