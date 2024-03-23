TARGET = shog
SRC_DIR = ./src
NODE_SRC_DIR = ./src/node

TARGET_DIR = ./target

OBJ_DIR = $(TARGET_DIR)

# compiler and linker
ifndef CC
CC = gcc
endif
LD = $(CC)

# flags
CFLAGS = -g -std=c11 -D_GNU_SOURCE -Wno-unused-value -Wno-format-zero-length $$(pkg-config --cflags openssl)  -I ./netlibc/include
CFLAGS_FLAT = -DNDEBUG

ifdef ANDROID
CFLAGS += -D__android__
endif

ifeq ($(CC), gcc)
CFLAGS += -Wno-format-overflow -Wno-format-truncation
else ifeq (gcc, $(shell if [ "$$(cc --help 2>&1 | grep -o -m 1 'gcc')" = "gcc" ]; then echo "gcc" ; elif [ "$$(cc --help 2>&1 | grep -o -m 1 'clang')" = "clang" ]; then echo "clang"; else echo "NONE"; fi))
CFLAGS += -Wno-format-overflow -Wno-format-truncation
endif

LDFLAGS = $$(pkg-config --libs openssl) $$(pkg-config --cflags --libs uuid) -pthread -lm

# warning flags
WARN_CFLAGS += -Werror -Wall -Wextra -Wformat -Wformat-security -Warray-bounds -Wconversion
# warning flags but will not stop compiling if a warning occurs
WARN_CFLAGS_NOSTOP += -Werror -Wall -Wextra -Wformat -Wformat-security -Warray-bounds

# sanitizers (only on linux)
ifeq ($(shell uname), Linux)
CFLAGS_SANITIZE = -fsanitize=address,undefined,leak,undefined 
LDFLAGS_SANITIZE += -lasan -lubsan
endif

MAIN_OBJ = $(TARGET_DIR)/main.o

# object files for node
# OBJS += $(TARGET_DIR)/args.o
OBJS += $(TARGET_DIR)/node.o $(TARGET_DIR)/studio.o $(TARGET_DIR)/utils.o $(TARGET_DIR)/openssl.o $(TARGET_DIR)/db.o $(TARGET_DIR)/tunnel.o $(TARGET_DIR)/json.o $(TARGET_DIR)/toml.o $(TARGET_DIR)/api.o
OBJS += $(TARGET_DIR)/args.o $(TARGET_DIR)/server.o $(TARGET_DIR)/manifest.o $(TARGET_DIR)/pins.o $(TARGET_DIR)/dht.o $(TARGET_DIR)/og.o
OBJS += $(TARGET_DIR)/server_explorer.o

# static libraries
STATIC_LIBS = $(TARGET_DIR)/libnetlibc.a $(TARGET_DIR)/sonic.a $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/libhandlebazz.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a $(TARGET_DIR)/libshogdb.a
STATIC_LIBS_SANITIZED = $(TARGET_DIR)/libnetlibc.a $(TARGET_DIR)/sonic-sanitized.a $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/libhandlebazz.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a $(TARGET_DIR)/libshogdb-sanitized.a

.PHONY: target-dir shogdb model-server $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/camel.a 
.PHONY: $(TARGET_DIR)/sonic.a $(TARGET_DIR)/sonic-sanitized.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a
.PHONY: build-objects-debug

check-cc:
	echo $(shell if [ "$$(cc --help 2>&1 | grep -o -m 1 'gcc')" = "gcc" ]; then echo "gcc" ; elif [ "$$(cc --help 2>&1 | grep -o -m 1 'clang')" = "clang" ]; then echo "clang"; else echo "NONE"; fi)

echo-objs:
	echo $(OBJS)

echo-static-libs:
	echo $(STATIC_LIBS)

# create target directories
target-dir:
	mkdir -p $(TARGET_DIR)

configure-ubuntu:
	sudo apt install build-essential libssl-dev uuid-dev pkgconf clang


# DEPENDENCIES
$(TARGET_DIR)/tuwi.a:
	cd ./tuwi/ && make
	cp ./tuwi/target/libtuwi.a $(TARGET_DIR)/tuwi.a
	
$(TARGET_DIR)/libhandlebazz.a:
	cd ./handlebazz/ && make
	cp ./handlebazz/target/libhandlebazz.a $(TARGET_DIR)/libhandlebazz.a

$(TARGET_DIR)/libnetlibc.a:
	cd ./netlibc/ && make
	cp ./netlibc/target/libnetlibc.a $(TARGET_DIR)/libnetlibc.a

camel: $(TARGET_DIR)/camel.a

$(TARGET_DIR)/camel.a: $(TARGET_DIR)/tuwi.a
	cd ./camel/ && make
	cp ./camel/target/libcamel.a $(TARGET_DIR)/camel.a

$(TARGET_DIR)/sonic.a:
	cd ./sonic/ && make
	cp ./sonic/target/libsonic.a $(TARGET_DIR)/sonic.a

$(TARGET_DIR)/libshogdb.a:
	cd ./shogdb/ && make lib
	cp ./shogdb/target/libshogdb.a $(TARGET_DIR)/libshogdb.a

$(TARGET_DIR)/libshogdb-sanitized.a:
	cd ./shogdb/ && make lib-sanitized
	cp ./shogdb/target/libshogdb-sanitized.a $(TARGET_DIR)/libshogdb-sanitized.a

$(TARGET_DIR)/sonic-sanitized.a:
	cd ./sonic/ && make build-sanitized
	cp ./sonic/target/libsonic-sanitized.a $(TARGET_DIR)/sonic-sanitized.a

$(TARGET_DIR)/cjson.a:
	cd ./lib/cjson/ && make
	cp ./lib/cjson/libcjson.a $(TARGET_DIR)/cjson.a

$(TARGET_DIR)/tomlc.a:
	cd ./lib/tomlc/ && make
	cp ./lib/tomlc/libtoml.a $(TARGET_DIR)/tomlc.a


$(TARGET_DIR)/shogdb:
	cd ./shogdb/ && make
	cp ./shogdb/target/shogdb $(TARGET_DIR)/

$(TARGET_DIR)/model_server:
	echo "Building model server..."
	cd ./lib/llamacpp/ && make server > /dev/null
	cp ./lib/llamacpp/server $(TARGET_DIR)/model_server

$(TARGET_DIR)/tunnel:
	echo "Building tunnel..."
	cd ./lib/bore/ && cargo build --release > /dev/null
	cp ./lib/bore/target/release/bore $(TARGET_DIR)/tunnel


dev: package-dev

shogdb: $(TARGET_DIR)/shogdb
model-server: $(TARGET_DIR)/model_server
tunnel: $(TARGET_DIR)/tunnel

build-sanitized: target-dir build-objects-sanitized link-objects-sanitized 
build-debug: target-dir build-objects-debug link-objects-debug
build-flat: target-dir build-objects-flat link-objects-flat
build-nowarn: target-dir build-objects-nowarn link-objects-nowarn

build-static: target-dir build-objects-debug
	echo "Creating Shoggoth static lib (debug)..."
	
	ar rcs $(TARGET_DIR)/libshoggoth.a $(OBJS)

build-objects-debug:
	echo "Building Shoggoth (debug)..."

	$(CC) $(CFLAGS) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/args/args.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/json/json.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/toml/toml.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -U _WIN32 -Wno-deprecated-declarations -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(NODE_SRC_DIR)/node.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/db/db.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/tunnel/tunnel.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/manifest/manifest.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/pins/pins.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -Wno-error=sign-conversion -Wno-error=conversion -c $(NODE_SRC_DIR)/og/og.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/api.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server_explorer.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/dht/dht.c
		
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/studio/studio.c
	
	mv ./*.o $(TARGET_DIR)

link-objects-debug: $(STATIC_LIBS)
	echo "Linking Shoggoth (debug)..."
	
	$(LD) $(OBJS) $(MAIN_OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $(TARGET_DIR)/$(TARGET)-dev 



build-objects-sanitized:
	echo "Building Shoggoth (sanitized)..."

	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -DVERSION="\"$(VERSION)\"" -c $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/json/json.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/toml/toml.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/args/args.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -U _WIN32 -Wno-deprecated-declarations $(CFLAGS_SANITIZE) -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -DVERSION="\"$(VERSION)\"" -c $(NODE_SRC_DIR)/node.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/db/db.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/tunnel/tunnel.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/server/server.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/manifest/manifest.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/pins/pins.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -Wno-error=sign-conversion -Wno-error=conversion $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/og/og.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/server/api.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/server/server_explorer.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/dht/dht.c
	
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/studio/studio.c
	
	mv ./*.o $(TARGET_DIR)

link-objects-sanitized: $(STATIC_LIBS_SANITIZED)
	echo "Linking Shoggoth (sanitized)..."
	
	$(LD) $(OBJS) $(MAIN_OBJ) $(LDFLAGS_SANITIZE) $(STATIC_LIBS_SANITIZED) $(LDFLAGS) -o $(TARGET_DIR)/$(TARGET)-sanitized 



build-objects-flat:
	echo "Building Shoggoth (flat)..."

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/args/args.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/json/json.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/toml/toml.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -U _WIN32 -Wno-deprecated-declarations -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(NODE_SRC_DIR)/node.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/db/db.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/tunnel/tunnel.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/manifest/manifest.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/pins/pins.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -Wno-error=sign-conversion -Wno-error=conversion -c $(NODE_SRC_DIR)/og/og.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/api.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server_explorer.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/dht/dht.c

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/studio/studio.c
	
	mv ./*.o $(TARGET_DIR)

link-objects-flat: $(STATIC_LIBS)
	echo "Linking Shoggoth (flat)..."
	
	$(LD) $(OBJS) $(MAIN_OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $(TARGET_DIR)/$(TARGET)-flat 
	strip $(TARGET_DIR)/$(TARGET)-flat



