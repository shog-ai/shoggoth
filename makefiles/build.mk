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
CFLAGS = -g -std=c11 -D_GNU_SOURCE -Wno-unused-value -Wno-format-zero-length $$(pkg-config --cflags openssl)
CFLAGS_FLAT = -DNDEBUG

ifeq ($(CC), gcc)
CFLAGS += -Wno-format-overflow -Wno-format-truncation
else ifeq (gcc, $(shell if [ "$$(cc --help 2>&1 | grep -o -m 1 'gcc')" = "gcc" ]; then echo "gcc" ; elif [ "$$(cc --help 2>&1 | grep -o -m 1 'clang')" = "clang" ]; then echo "clang"; else echo "NONE"; fi))
CFLAGS += -Wno-format-overflow -Wno-format-truncation
endif

LDFLAGS = $$(pkg-config --libs openssl) $$(pkg-config --cflags --libs uuid) -lnetlibc

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
OBJS += $(TARGET_DIR)/node.o $(TARGET_DIR)/utils.o $(TARGET_DIR)/openssl.o $(TARGET_DIR)/db.o $(TARGET_DIR)/redis.o $(TARGET_DIR)/json.o $(TARGET_DIR)/toml.o $(TARGET_DIR)/api.o
OBJS += $(TARGET_DIR)/args.o $(TARGET_DIR)/server.o $(TARGET_DIR)/manifest.o $(TARGET_DIR)/pins.o $(TARGET_DIR)/pin.o $(TARGET_DIR)/dht.o $(TARGET_DIR)/garbage_collector.o
OBJS += $(TARGET_DIR)/server_explorer.o $(TARGET_DIR)/server_profile.o $(TARGET_DIR)/templating.o

# object files for client
OBJS += $(TARGET_DIR)/client.o $(TARGET_DIR)/profile.o $(TARGET_DIR)/clone.o
OBJS +=
OBJS +=

# static libraries
STATIC_LIBS = $(TARGET_DIR)/sonic.a $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a
STATIC_LIBS_SANITIZED = $(TARGET_DIR)/sonic-sanitized.a $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a

.PHONY: target-dir redis $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/camel.a 
.PHONY: $(TARGET_DIR)/sonic.a $(TARGET_DIR)/sonic-sanitized.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a
.PHONY: build-objects-debug

check-cc:
	echo $(shell if [ "$$(cc --help 2>&1 | grep -o -m 1 'gcc')" = "gcc" ]; then echo "gcc" ; elif [ "$$(cc --help 2>&1 | grep -o -m 1 'clang')" = "clang" ]; then echo "clang"; else echo "NONE"; fi)

check-netlibc:
	cd ./lib/netlibc/ && make && make check-netlibc

echo-objs:
	echo $(OBJS)

echo-static-libs:
	echo $(STATIC_LIBS)

# create target directories
target-dir:
	mkdir -p $(TARGET_DIR)

setup-linux:
	sudo apt install build-essential libssl-dev uuid-dev pkgconf


# LIBRARIES
$(TARGET_DIR)/tuwi.a:
	cd ./lib/tuwi/ && make
	cp ./lib/tuwi/target/libtuwi.a $(TARGET_DIR)/tuwi.a

camel: $(TARGET_DIR)/camel.a

$(TARGET_DIR)/camel.a: $(TARGET_DIR)/tuwi.a
	cd ./lib/camel/ && make
	cp ./lib/camel/target/libcamel.a $(TARGET_DIR)/camel.a

$(TARGET_DIR)/sonic.a:
	cd ./lib/sonic/ && make
	cp ./lib/sonic/target/libsonic.a $(TARGET_DIR)/sonic.a

$(TARGET_DIR)/sonic-sanitized.a:
	cd ./lib/sonic/ && make build-sanitized
	cp ./lib/sonic/target/libsonic-sanitized.a $(TARGET_DIR)/sonic-sanitized.a

$(TARGET_DIR)/cjson.a:
	cd ./lib/cjson/ && make
	cp ./lib/cjson/libcjson.a $(TARGET_DIR)/cjson.a

$(TARGET_DIR)/tomlc.a:
	cd ./lib/tomlc/ && make
	cp ./lib/tomlc/libtoml.a $(TARGET_DIR)/tomlc.a

$(TARGET_DIR)/redisjson.so:
	echo "Building redisjson..."

ifeq (, $(shell which cargo))
	echo "A dependency \"redisjson\" requires the Rust programming language to be installed"
	echo "choose one of the following options to proceed (recommend option 1) :"
	echo "1. Download a precompiled binary of \"redisjson\" without installing Rust, and continue the build"
	echo "2. Automatically install Rust and build \"redisjson\" from source using ./lib/redisjson/"
	echo "3. Cancel the build and exit"
	@read -p "Enter your choice: " choice; \
	if [ "$$choice" = "1" ]; then \
		if [ $(shell uname -s) = Darwin ] && [ $(shell uname -m) = x86_64 ]; then \
			echo "Downloading redisjson for Macos x86_64 from https://shoggoth.systems/download/redisjson-Darwin-x86_64.so "; \
			wget -O ./target/redisjson.so https://shoggoth.systems/download/redisjson-Darwin-x86_64.so ; \
		elif [ $(shell uname -s) = Linux ] && [ $(shell uname -m) = x86_64 ]; then \
			echo "Downloading redisjson for Linux x86_64 from https://shoggoth.systems/download/redisjson-Linux-x86_64.so "; \
			wget -O ./target/redisjson.so https://shoggoth.systems/download/redisjson-Linux-x86_64.so ; \
		fi \
	elif [ "$$choice" = "2" ]; then \
		echo "Installing Rust with rustup ..."; \
		curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh ; \
		echo "then run the make command again to continue building Shoggoth" ; \
		exit 1; \
	elif [ "$$choice" = "3" ]; then \
		echo "Cancel build ..."; \
		exit 11; \
	else \
		echo "Invalid choice: $$choice"; \
		exit 11; \
	fi
else
	cd ./lib/redisjson/ && cargo build --release --quiet

	if [ $(shell uname -s) = Darwin ]; then \
		cp ./lib/redisjson/target/release/librejson.dylib $(TARGET_DIR)/redisjson.so ; \
	elif [ $(shell uname -s) = Linux ]; then \
		cp ./lib/redisjson/target/release/librejson.so $(TARGET_DIR)/redisjson.so ; \
	fi
endif
	

$(TARGET_DIR)/redis-server:
	echo "Building redis..."
	
	cd ./lib/redis/ && make MALLOC=libc
	cp ./lib/redis/src/redis-server $(TARGET_DIR)/
	cp ./lib/redis/src/redis-cli $(TARGET_DIR)/

redis: $(TARGET_DIR)/redis-server

build-dynamic-libs: $(TARGET_DIR)/redisjson.so

dev: package-dev

build: check-netlibc build-flat redis
build-dev: check-netlibc build-sanitized redis

build-sanitized: target-dir build-dynamic-libs build-objects-sanitized link-objects-sanitized 
build-debug: target-dir build-dynamic-libs build-objects-debug link-objects-debug
build-flat: target-dir build-dynamic-libs build-objects-flat link-objects-flat
build-nowarn: target-dir build-dynamic-libs build-objects-nowarn link-objects-nowarn

build-static: target-dir build-objects-debug
	echo "Creating Shoggoth static lib (debug)..."
	
	ar rcs $(TARGET_DIR)/libshoggoth.a $(OBJS)

build-objects-debug:
	echo "Building Shoggoth (debug)..."

	$(CC) $(CFLAGS) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/args/args.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/redis/redis.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/json/json.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/toml/toml.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/templating/templating.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/profile/profile.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/utils/utils.c
#disabled deprecation warnings
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -Wno-deprecated-declarations -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/client.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/clone.c

	$(CC) $(CFLAGS) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(NODE_SRC_DIR)/node.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/db/db.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/manifest/manifest.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/pins/pins.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/garbage_collector/garbage_collector.c
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
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/redis/redis.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/json/json.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/toml/toml.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/templating/templating.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/args/args.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/profile/profile.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/utils/utils.c
#disabled deprecation warnings
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -Wno-deprecated-declarations $(CFLAGS_SANITIZE) -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(CLIENT_SRC_DIR)/client.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(CLIENT_SRC_DIR)/clone.c

	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -DVERSION="\"$(VERSION)\"" -c $(NODE_SRC_DIR)/node.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/db/db.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/server/server.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/manifest/manifest.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/pins/pins.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(NODE_SRC_DIR)/garbage_collector/garbage_collector.c
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
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/redis/redis.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/profile/profile.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/utils/utils.c
#disable deprecation warnings
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -Wno-deprecated-declarations -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/client.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/clone.c

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(NODE_SRC_DIR)/node.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/db/db.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/server/server.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/manifest/manifest.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/pins/pins.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(NODE_SRC_DIR)/garbage_collector/garbage_collector.c
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



