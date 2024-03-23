VERSION = 0.1.4

MAKEFLAGS += --silent

ifndef CC
CC = gcc
endif

CFLAGS = -g -I ../netlibc/include

all: ./target/libhandlebazz.a

./target/libhandlebazz.a: target-dir
	$(CC) $(CFLAGS) -c ./src/handlebazz.c -o ./target/handlebazz.o
	ar rcs ./target/libhandlebazz.a ./target/handlebazz.o

target-dir:
	mkdir -p target


clean:
	rm -rf ./target/

downstream:
	git fetch && git pull
	
upstream:
	git add .
	@read -p "Enter commit message: " message; \
	git commit -m "$$message"
	git push

