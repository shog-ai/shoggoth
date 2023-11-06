VERSION = 0.1.4

MAKEFLAGS += --silent

ifndef CC
CC = gcc
endif

all: ./target/libnetlibc.a

install:
	sudo cp ./netlibc.h /usr/local/include
	sudo cp ./target/libnetlibc.a /usr/local/lib

uninstall:
	sudo rm /usr/local/include/netlibc.h
	sudo rm /usr/local/lib/libnetlibc.a

./target/libnetlibc.a: target-dir
	$(CC) -c ./src/netlibc.c -o ./target/netlibc.o
	ar rcs ./target/libnetlibc.a ./target/netlibc.o

target-dir:
	mkdir -p target
	mkdir -p target/examples


run-example: target-dir ./target/libnetlibc.a
	$(CC) $(CFLAGS) ./examples/$(E).c ./target/libnetlibc.a -o ./target/examples/$(E)
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

