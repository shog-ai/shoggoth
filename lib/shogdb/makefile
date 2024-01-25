VERSION = 0.1.4

MAKEFLAGS += --silent

ifndef CC
CC = gcc
endif

CFLAGS += -g -std=c11 -D_GNU_SOURCE -I ../netlibc/include $$(pkg-config --cflags openssl) 
LDFLAGS += -pthread $$(pkg-config --libs openssl)

# sanitizers (only on linux)
ifeq ($(shell uname), Linux)
CFLAGS_SANITIZED += -fsanitize=address,undefined,leak
LDFLAGS_SANITIZED += -lasan -lubsan
endif

STATIC_LIBS = ../../target/cjson.a ../../target/tomlc.a ../../target/sonic.a ../../target/libnetlibc.a

all: build

build: ./target/shogdb
	cp ./src/dbconfig.toml ./target/dbconfig.toml
	

lib: target-dir
	echo "Building libshogdb..."

	$(CC) $(CFLAGS) -c ./src/lib/lib.c -o ./target/lib.o
	ar rcs ./target/libshogdb.a ./target/lib.o

lib-sanitized: target-dir
	echo "Building libshogdb..."
	$(CC) $(CFLAGS) $(CFLAGS_SANITIZED) -c ./src/lib/lib.c -o ./target/lib.o
	ar rcs ./target/libshogdb-sanitized.a ./target/lib.o

run: build
	cd target && ./shogdb

./target/shogdb: target-dir
	echo "Building shogdb..."
	
	$(CC) $(CFLAGS) ./src/main.c ./src/lib/lib.c ./src/db/db.c ./src/db/dht.c ./src/db/pins.c ./src/hashmap/hashmap.c $(STATIC_LIBS) $(LDFLAGS) -o ./target/shogdb

./target/shogdb-sanitized: target-dir
	echo "Building shogdb..."
	
	$(CC) $(CFLAGS_SANITIZED) $(CFLAGS) $(LDFLAGS_SANITIZED) ./src/main.c ./src/lib/lib.c ./src/db/db.c ./src/db/dht.c ./src/db/pins.c ./src/hashmap/hashmap.c $(STATIC_LIBS) $(LDFLAGS) -o ./target/shogdb

target-dir:
	mkdir -p target
	mkdir -p target/examples

clean:
	rm -rf ./target/

sync:
	cp ../sonic/sonic.h ./include/sonic.h
