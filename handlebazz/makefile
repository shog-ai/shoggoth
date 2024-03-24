VERSION = 0.1.4

MAKEFLAGS += --silent

ifndef CC
CC = gcc
endif

CFLAGS = -g -I ../netlibc/include
LDFLAGS = -lnetlibc

STATIC_LIBS = ./target/libcjson.a

all: ./target/libhandlebazz.a

./target/libhandlebazz.a: target-dir $(STATIC_LIBS)
	$(CC) $(CFLAGS) -c ./src/handlebazz.c -o ./target/handlebazz.o
	ar rcs ./target/libhandlebazz.a ./target/handlebazz.o

run-example: ./target/libhandlebazz.a 
	$(CC) $(CFLAGS) ./examples/$(E).c ./target/libhandlebazz.a $(STATIC_LIBS) $(LDFLAGS) -o ./target/examples/$(E)
	cd target/examples/ && ./$(E)

target-dir:
	mkdir -p ./target
	mkdir -p ./target/examples


./target/libcjson.a:
	cd ../lib/cjson/ && $(MAKE)
	cp ../lib/cjson/libcjson.a ./target/libcjson.a

clean:
	rm -rf ./target/

downstream:
	git fetch && git pull
	
upstream:
	git add .
	@read -p "Enter commit message: " message; \
	git commit -m "$$message"
	git push

