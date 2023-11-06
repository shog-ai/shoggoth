VERSION = 0.1.4

MAKEFLAGS += --silent

ifndef CC
CC = gcc
endif

all: ./target/libtuwi.a

./target/libtuwi.a: target-dir netlibc
	$(CC) -c ./src/tuwi.c -o ./target/tuwi.o
	ar rcs ./target/libtuwi.a ./target/tuwi.o

netlibc:
ifeq (,$(wildcard /usr/local/include/netlibc.h ))
	echo "Installing netlibc ..."
	cd ../netlibc/ && make && make install
endif

target-dir:
	mkdir -p target
	mkdir -p target/examples


run-example: target-dir ./target/libtuwi.a
	$(CC) $(CFLAGS) ./examples/$(E).c ./target/libtuwi.a -o ./target/examples/$(E)
	cd target/examples/ && ./$(E)

clean:
	rm -rf ./target/

downstream:
	git fetch && git pull
	
upstream:
	git add .
	@read -p "Enter commit message: " message; \
	git commit -m "$$message"
	git push

