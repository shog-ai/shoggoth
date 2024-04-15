VERSION = 0.1.4

MAKEFLAGS += --silent

ifndef CC
CC = gcc
endif

CFLAGS = -g

ifdef ANDROID
CFLAGS += -D__android__
endif

.PHONY: ./target/libnetlibc.a

all: ./target/libnetlibc.a

install:
	sudo mkdir -p /usr/local/include/netlibc/
	
	sudo cp -r ./include/netlibc/* /usr/local/include/netlibc/
	sudo cp ./include/netlibc.h /usr/local/include/netlibc.h

	sudo cp ./target/libnetlibc.a /usr/local/lib

uninstall:
	sudo rm -r /usr/local/include/netlibc/
	sudo rm /usr/local/include/netlibc.h
	
	sudo rm /usr/local/lib/libnetlibc.a

./target/libnetlibc.a: target-dir
	$(CC) $(CFLAGS) -c ./src/netlibc.c -o ./target/netlibc.o
	$(CC) $(CFLAGS) -c ./src/error.c -o ./target/error.o
	$(CC) $(CFLAGS) -c ./src/fs.c -o ./target/fs.o
	$(CC) $(CFLAGS) -c ./src/log.c -o ./target/log.o
	$(CC) $(CFLAGS) -c ./src/string.c -o ./target/string.o
	$(CC) $(CFLAGS) -c ./src/assert.c -o ./target/assert.o
	 $(CC) $(CFLAGS) -c ./src/mem.c -o ./target/mem.o
	
	ar rcs ./target/libnetlibc.a ./target/netlibc.o ./target/error.o ./target/fs.o ./target/log.o ./target/string.o ./target/assert.o ./target/mem.o

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

