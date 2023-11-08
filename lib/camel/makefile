VERSION = 0.1.4

MAKEFLAGS += --silent

CFLAGS := -g -I ../netlibc/include -Wall -Wextra -Wformat -Wformat-security -Warray-bounds

ifndef CC
CC = gcc
endif

.PHONY: ./target/tuwi.o ./target/libcamel.a

all: ./target/libcamel.a

setup:

./target/libcamel.a: target-dir ./target/tuwi.o ./camel.h ./src/camel.c
	$(CC) $(CFLAGS) -c ./src/camel.c -o ./target/camel.o
	ar rcs ./target/libcamel.a ./target/camel.o ./target/tuwi.o

./target/tuwi.o:
	cd ../tuwi/ && make
	cp ../tuwi/target/libtuwi.a ./target/tuwi.a
	cd target && ar x ./tuwi.a

target-dir:
	mkdir -p target
	mkdir -p target/examples

run-example: target-dir ./target/libcamel.a prepare-examples
	$(CC) $(CFLAGS) ./examples/$(E)/$(E).c ./examples/$(E)/math.c ./target/libcamel.a -o ./target/examples/$(E)
	cd target/examples/ && ./$(E)

prepare-examples:
	echo "Hello world" > ./target/examples/hello.txt
	cp ./target/examples/hello.txt ./target/examples/hello_copy.txt

clean:
	rm -rf ./target/*

sync: sync-libs-headers

sync-libs-headers:
	cp ../tuwi/tuwi.h ./include/tuwi.h

# sync-tuwi:
# 	rm -rf ./lib/tuwi/
# 	rsync -av --exclude='target' --exclude='.git' ../tuwi/ ./lib/tuwi/

downstream:
	git fetch && git pull
	
upstream:
	git add .
	@read -p "Enter commit message: " message; \
	git commit -m "$$message"
	git push

