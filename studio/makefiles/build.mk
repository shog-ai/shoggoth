TARGET = studio
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
CFLAGS = -g -std=c11 -D_GNU_SOURCE -Wno-unused-value -Wno-format-zero-length $$(pkg-config --cflags openssl)  -I ../netlibc/include
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
CFLAGS_SANITIZE += -DMEM_DEBUG

MAIN_OBJ = $(TARGET_DIR)/main.o

# object files for node
OBJS += $(TARGET_DIR)/studio.o $(TARGET_DIR)/utils.o $(TARGET_DIR)/json.o 
OBJS += $(TARGET_DIR)/args.o

STATIC_LIBS = $(TARGET_DIR)/libnetlibc.a $(TARGET_DIR)/libhandlebazz.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/sonic.a
STATIC_LIBS_SANITIZED = $(TARGET_DIR)/libnetlibc.a $(TARGET_DIR)/sonic-sanitized.a $(TARGET_DIR)/libhandlebazz.a $(TARGET_DIR)/cjson.a
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
	
$(TARGET_DIR)/libhandlebazz.a:
	cd ../handlebazz/ && make
	cp ../handlebazz/target/libhandlebazz.a $(TARGET_DIR)/libhandlebazz.a

$(TARGET_DIR)/libnetlibc.a:
	cd ../netlibc/ && make
	cp ../netlibc/target/libnetlibc.a $(TARGET_DIR)/libnetlibc.a

$(TARGET_DIR)/sonic.a:
	cd ../sonic/ && make
	cp ../sonic/target/libsonic.a $(TARGET_DIR)/sonic.a

$(TARGET_DIR)/sonic-sanitized.a:
	cd ../sonic/ && make build-sanitized
	cp ../sonic/target/libsonic-sanitized.a $(TARGET_DIR)/sonic-sanitized.a

$(TARGET_DIR)/cjson.a:
	cd ../lib/cjson/ && make
	cp ../lib/cjson/libcjson.a $(TARGET_DIR)/cjson.a

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
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/json/json.c
		
	$(CC) $(CFLAGS) $(WARN_CFLAGS) -c $(SRC_DIR)/studio.c
	
	mv ./*.o $(TARGET_DIR)

link-objects-debug: $(STATIC_LIBS)
	echo "Linking Shoggoth (debug)..."
	
	$(LD) $(OBJS) $(MAIN_OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $(TARGET_DIR)/$(TARGET)-dev 



build-objects-sanitized:
	echo "Building Shoggoth (sanitized)..."

	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -DVERSION="\"$(VERSION)\"" -c $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/args/args.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/json/json.c
	
	$(CC) $(CFLAGS) $(WARN_CFLAGS) $(CFLAGS_SANITIZE) -c $(SRC_DIR)/studio.c
	
	mv ./*.o $(TARGET_DIR)

link-objects-sanitized: $(STATIC_LIBS_SANITIZED)
	echo "Linking Shoggoth (sanitized)..."
	
	$(LD) $(OBJS) $(MAIN_OBJ) $(LDFLAGS_SANITIZE) $(STATIC_LIBS_SANITIZED) $(LDFLAGS) -o $(TARGET_DIR)/$(TARGET)-sanitized 



build-objects-flat:
	echo "Building Shoggoth (flat)..."

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/args/args.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/json/json.c

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/studio.c
	
	mv ./*.o $(TARGET_DIR)

link-objects-flat: $(STATIC_LIBS)
	echo "Linking Shoggoth (flat)..."
	
	$(LD) $(OBJS) $(MAIN_OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $(TARGET_DIR)/$(TARGET)-flat 
	strip $(TARGET_DIR)/$(TARGET)-flat



