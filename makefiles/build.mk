TARGET = shog
SRC_DIR = ./src
NODE_SRC_DIR = ./src/node
CLIENT_SRC_DIR = ./src/client

TARGET_DIR = ./target

OBJ_DIR = $(TARGET_DIR)

# compiler and linker
ifndef CC
CC = gcc
endif
LD = $(CC)

# flags
CFLAGS = -g -std=c11 -D_GNU_SOURCE -Wno-unused-value -Wno-format-zero-length $$(pkg-config --cflags openssl) $$(pkg-config --cflags MagickWand) -I ./lib/netlibc/include
CFLAGS_FLAT = -DNDEBUG

ifeq ($(CC), gcc)
CFLAGS += -Wno-format-overflow -Wno-format-truncation
else ifeq (gcc, $(shell if [ "$$(cc --help 2>&1 | grep -o -m 1 'gcc')" = "gcc" ]; then echo "gcc" ; elif [ "$$(cc --help 2>&1 | grep -o -m 1 'clang')" = "clang" ]; then echo "clang"; else echo "NONE"; fi))
CFLAGS += -Wno-format-overflow -Wno-format-truncation
endif

ifeq ($(shell pkg-config --exists 'MagickWand >= 7' && echo "YES" || echo "NO"), YES)
    CFLAGS += -DMAGICKWAND_7
endif

LDFLAGS = $$(pkg-config --libs openssl) $$(pkg-config --cflags --libs uuid) $$(pkg-config --libs MagickWand) -pthread

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
OBJS += $(TARGET_DIR)/node.o $(TARGET_DIR)/utils.o $(TARGET_DIR)/openssl.o $(TARGET_DIR)/db.o $(TARGET_DIR)/json.o $(TARGET_DIR)/toml.o $(TARGET_DIR)/api.o
OBJS += $(TARGET_DIR)/args.o $(TARGET_DIR)/server.o $(TARGET_DIR)/manifest.o $(TARGET_DIR)/pins.o $(TARGET_DIR)/pin.o $(TARGET_DIR)/dht.o $(TARGET_DIR)/garbage_collector.o $(TARGET_DIR)/og.o
OBJS += $(TARGET_DIR)/server_explorer.o $(TARGET_DIR)/server_profile.o $(TARGET_DIR)/templating.o

# object files for client
OBJS += $(TARGET_DIR)/client.o $(TARGET_DIR)/profile.o $(TARGET_DIR)/clone.o
OBJS +=
OBJS +=

# static libraries
STATIC_LIBS = $(TARGET_DIR)/libnetlibc.a $(TARGET_DIR)/sonic.a $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a $(TARGET_DIR)/libshogdb.a
STATIC_LIBS_SANITIZED = $(TARGET_DIR)/libnetlibc.a $(TARGET_DIR)/sonic-sanitized.a $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a $(TARGET_DIR)/libshogdb-sanitized.a

.PHONY: target-dir shogdb $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/camel.a 
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
	sudo apt install build-essential libssl-dev uuid-dev pkgconf clang libmagickwand-dev


# DEPENDENCIES
$(TARGET_DIR)/tuwi.a:
	cd ./lib/tuwi/ && make
	cp ./lib/tuwi/target/libtuwi.a $(TARGET_DIR)/tuwi.a

$(TARGET_DIR)/libnetlibc.a:
	cd ./lib/netlibc/ && make
	cp ./lib/netlibc/target/libnetlibc.a $(TARGET_DIR)/libnetlibc.a

camel: $(TARGET_DIR)/camel.a

$(TARGET_DIR)/camel.a: $(TARGET_DIR)/tuwi.a
	cd ./lib/camel/ && make
	cp ./lib/camel/target/libcamel.a $(TARGET_DIR)/camel.a

$(TARGET_DIR)/sonic.a:
	cd ./lib/sonic/ && make
	cp ./lib/sonic/target/libsonic.a $(TARGET_DIR)/sonic.a

$(TARGET_DIR)/libshogdb.a:
	cd ./lib/shogdb/ && make lib
	cp ./lib/shogdb/target/libshogdb.a $(TARGET_DIR)/libshogdb.a

$(TARGET_DIR)/libshogdb-sanitized.a:
	cd ./lib/shogdb/ && make lib-sanitized
	cp ./lib/shogdb/target/libshogdb-sanitized.a $(TARGET_DIR)/libshogdb-sanitized.a

$(TARGET_DIR)/sonic-sanitized.a:
	cd ./lib/sonic/ && make build-sanitized
	cp ./lib/sonic/target/libsonic-sanitized.a $(TARGET_DIR)/sonic-sanitized.a

$(TARGET_DIR)/cjson.a:
	cd ./lib/cjson/ && make
	cp ./lib/cjson/libcjson.a $(TARGET_DIR)/cjson.a

$(TARGET_DIR)/tomlc.a:
	cd ./lib/tomlc/ && make
	cp ./lib/tomlc/libtoml.a $(TARGET_DIR)/tomlc.a


$(TARGET_DIR)/shogdb:
	cd ./lib/shogdb/ && make
	cp ./lib/shogdb/target/shogdb $(TARGET_DIR)/


dev: package-dev

shogdb: $(TARGET_DIR)/shogdb

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
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/templating/templating.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/profile/profile.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -U _WIN32 -Wno-deprecated-declarations -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/client.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/clone.c

	$(CC) $(CFLAGS) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(NODE_SRC_DIR)/node.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/db/db.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/manifest/manifest.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/pins/pins.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/garbage_collector/garbage_collector.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -Wno-error=sign-conversion -Wno-error=conversion -c $(NODE_SRC_DIR)/og/og.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/pin.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/api.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server_explorer.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server_profile.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/dht/dht.c
		
	mv ./*.o $(TARGET_DIR)

link-objects-debug: $(STATIC_LIBS)
	echo "Linking Shoggoth (debug)..."
	
	$(LD) $(OBJS) $(MAIN_OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $(TARGET_DIR)/$(TARGET)-dev 



build-objects-sanitized:
	echo "Building Shoggoth (sanitized)..."

	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -DVERSION="\"$(VERSION)\"" -c $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/json/json.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/toml/toml.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/templating/templating.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/args/args.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/profile/profile.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -U _WIN32 -Wno-deprecated-declarations $(CFLAGS_SANITIZE) -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(CLIENT_SRC_DIR)/client.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(CLIENT_SRC_DIR)/clone.c

	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -DVERSION="\"$(VERSION)\"" -c $(NODE_SRC_DIR)/node.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/db/db.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/server/server.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/manifest/manifest.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/pins/pins.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/garbage_collector/garbage_collector.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -Wno-error=sign-conversion -Wno-error=conversion $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/og/og.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/server/pin.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/server/api.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/server/server_explorer.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/server/server_profile.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/dht/dht.c
	
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
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/templating/templating.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/profile/profile.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -U _WIN32 -Wno-deprecated-declarations -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/client.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/clone.c

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(NODE_SRC_DIR)/node.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/db/db.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/manifest/manifest.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/pins/pins.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/garbage_collector/garbage_collector.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -Wno-error=sign-conversion -Wno-error=conversion -c $(NODE_SRC_DIR)/og/og.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/pin.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/api.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server_explorer.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server_profile.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/dht/dht.c

	mv ./*.o $(TARGET_DIR)

link-objects-flat: $(STATIC_LIBS)
	echo "Linking Shoggoth (flat)..."
	
	$(LD) $(OBJS) $(MAIN_OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $(TARGET_DIR)/$(TARGET)-flat 
	strip $(TARGET_DIR)/$(TARGET)-flat



