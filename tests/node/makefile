MAKEFLAGS += --silent

CFLAGS = -g -std=c11

# warning flags
WARN_CFLAGS = -Wall -Wextra -Wformat -Wformat-security -Warray-bounds -Wconversion
# WARN_CFLAGS += -Werror

ifeq ($(shell uname), Linux)
CFLAGS += -fsanitize=address,undefined,leak,undefined
LDFLAGS += -lasan -lubsan 
endif

CFLAGS +=
LDFLAGS += -lssl -lcrypto $$(pkg-config --cflags --libs uuid) -lnetlibc

TARGET_DIR = ../../target

ROOT_DIR = ../../

# static libraries
STATIC_LIBS = $(TARGET_DIR)/sonic-sanitized.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a

CC = gcc
LD = gcc

.PHONY: build-functional-tests

camel:
	cd $(ROOT_DIR) && $(MAKE) camel

build-shoggoth-static:
	cd $(ROOT_DIR) && $(MAKE) build-static

target-dir:
	mkdir -p $(TARGET_DIR)
	
	mkdir -p $(TARGET_DIR)/tests
	mkdir -p $(TARGET_DIR)/tests/node/
	
	mkdir -p $(TARGET_DIR)/tests/node/functional
	mkdir -p $(TARGET_DIR)/tests/node/functional/discovery
	mkdir -p $(TARGET_DIR)/tests/node/functional/api
	mkdir -p $(TARGET_DIR)/tests/node/functional/publish
	mkdir -p $(TARGET_DIR)/tests/node/functional/pin

	mkdir -p $(TARGET_DIR)/tests/node/unit
	mkdir -p $(TARGET_DIR)/tests/node/integration
	mkdir -p $(TARGET_DIR)/tests/node/fuzz

# FUNCTIONAL ========================================================================================

prepare-shoggoth: 
	rm -r $(TARGET_DIR)/tests/node/functional/
	$(MAKE) target-dir
	
	cd $(ROOT_DIR) && $(MAKE) build-sanitized
	cp $(TARGET_DIR)/shog-sanitized $(TARGET_DIR)/tests/node/functional/api/shog 
	cp $(TARGET_DIR)/shog-sanitized $(TARGET_DIR)/tests/node/functional/publish/shog 
	cp $(TARGET_DIR)/shog-sanitized $(TARGET_DIR)/tests/node/functional/discovery/shog
	cp $(TARGET_DIR)/shog-sanitized $(TARGET_DIR)/tests/node/functional/pin/shog

# API
	cd $(ROOT_DIR) && $(MAKE) generate-runtime RP=./target/tests/node/functional/api/runtime/
	cd $(ROOT_DIR) && cp ./deploy/1/public.txt ./target/tests/node/functional/api/runtime/node/keys/ && cp ./deploy/1/private.txt ./target/tests/node/functional/api/runtime/node/keys/	
	cd $(ROOT_DIR) && cp ./tests/node/functional/config/node-config.toml ./target/tests/node/functional/api/runtime/node/config.toml

# PUBLISH
	cd $(ROOT_DIR) && $(MAKE) generate-runtime RP=./target/tests/node/functional/publish/runtime/
	
	cd $(ROOT_DIR) && cp ./deploy/1/public.txt ./target/tests/node/functional/publish/runtime/node/keys/ && cp ./deploy/1/private.txt ./target/tests/node/functional/publish/runtime/node/keys/	
	cd $(ROOT_DIR) && cp ./tests/node/functional/config/node-config.toml ./target/tests/node/functional/publish/runtime/node/config.toml
	
	cd $(ROOT_DIR) && cp ./tests/node/functional/config/client/keys/public.txt ./target/tests/node/functional/publish/runtime/client/keys/ && cp ./tests/node/functional/config/client/keys/private.txt ./target/tests/node/functional/publish/runtime/client/keys/	
	cd $(ROOT_DIR) && cp -r ./tests/node/functional/config/client/config.toml ./target/tests/node/functional/publish/runtime/client/config.toml

# DISCOVERY
	cd $(ROOT_DIR) && $(MAKE) generate-runtime RP=./target/tests/node/functional/discovery/node1/
	cp ./functional/config/1/public.txt $(ROOT_DIR)/target/tests/node/functional/discovery/node1/node/keys/ && cp ./functional/config/1/private.txt $(ROOT_DIR)/target/tests/node/functional/discovery/node1/node/keys/	
	cp ./functional/config/1/1.toml $(ROOT_DIR)/target/tests/node/functional/discovery/node1/node/config.toml

	cd $(ROOT_DIR) && $(MAKE) generate-runtime RP=./target/tests/node/functional/discovery/node2/
	cp ./functional/config/2/public.txt $(ROOT_DIR)/target/tests/node/functional/discovery/node2/node/keys/ && cp ./functional/config/2/private.txt $(ROOT_DIR)/target/tests/node/functional/discovery/node2/node/keys/	
	cp ./functional/config/2/2.toml $(ROOT_DIR)/target/tests/node/functional/discovery/node2/node/config.toml

	cd $(ROOT_DIR) && $(MAKE) generate-runtime RP=./target/tests/node/functional/discovery/node3/
	cp ./functional/config/3/public.txt $(ROOT_DIR)/target/tests/node/functional/discovery/node3/node/keys/ && cp ./functional/config/3/private.txt $(ROOT_DIR)/target/tests/node/functional/discovery/node3/node/keys/	
	cp ./functional/config/3/3.toml $(ROOT_DIR)/target/tests/node/functional/discovery/node3/node/config.toml


# PIN
	cd $(ROOT_DIR) && $(MAKE) generate-runtime RP=./target/tests/node/functional/pin/node1/
	cp ./functional/config/1/public.txt $(ROOT_DIR)/target/tests/node/functional/pin/node1/node/keys/ && cp ./functional/config/1/private.txt $(ROOT_DIR)/target/tests/node/functional/pin/node1/node/keys/	
	cp ./functional/config/1/1.toml $(ROOT_DIR)/target/tests/node/functional/pin/node1/node/config.toml

	cd $(ROOT_DIR) && $(MAKE) generate-runtime RP=./target/tests/node/functional/pin/node2/
	cp ./functional/config/2/public.txt $(ROOT_DIR)/target/tests/node/functional/pin/node2/node/keys/ && cp ./functional/config/2/private.txt $(ROOT_DIR)/target/tests/node/functional/pin/node2/node/keys/	
	cp ./functional/config/2/2.toml $(ROOT_DIR)/target/tests/node/functional/pin/node2/node/config.toml

	cd $(ROOT_DIR) && $(MAKE) generate-runtime RP=./target/tests/node/functional/pin/node3/
	cp ./functional/config/3/public.txt $(ROOT_DIR)/target/tests/node/functional/pin/node3/node/keys/ && cp ./functional/config/3/private.txt $(ROOT_DIR)/target/tests/node/functional/pin/node3/node/keys/	
	cp ./functional/config/3/3.toml $(ROOT_DIR)/target/tests/node/functional/pin/node3/node/config.toml

	cd $(ROOT_DIR) && $(MAKE) generate-runtime RP=./target/tests/node/functional/pin/client/
	cd $(ROOT_DIR) && cp ./tests/node/functional/config/client/keys/public.txt ./target/tests/node/functional/pin/client/client/keys/ && cp ./tests/node/functional/config/client/keys/private.txt ./target/tests/node/functional/pin/client/client/keys/	
	cd $(ROOT_DIR) && cp -r ./tests/node/functional/config/client/config.toml ./target/tests/node/functional/pin/client/client/config.toml

run-functional-tests:
	cd $(TARGET_DIR)/tests/node/functional/api/ && ./test
	cd $(TARGET_DIR)/tests/node/functional/publish/ && ./test
	cd $(TARGET_DIR)/tests/node/functional/discovery/ && ./test
	cd $(TARGET_DIR)/tests/node/functional/pin/ && ./test
	
build-functional-tests: target-dir prepare-shoggoth build-shoggoth-static camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./functional/api/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/node/functional/api/test
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./functional/publish/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/node/functional/publish/test
	
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./functional/discovery/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/node/functional/discovery/test
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./functional/pin/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/node/functional/pin/test


#========================================================================================


# UNIT ========================================================================================

run-unit-tests:
	cd $(TARGET_DIR)/tests/node/unit/ && ./test
	cd $(TARGET_DIR)/tests/node/unit/ && ./test_args
	cd $(TARGET_DIR)/tests/node/unit/ && ./test_utils
	
build-unit-tests: target-dir build-shoggoth-static camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./unit/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/node/unit/test
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./unit/test_args.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/node/unit/test_args
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./unit/test_utils.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/node/unit/test_utils

#========================================================================================

# INTEGRATION ========================================================================================

run-integration-tests:
	cd $(TARGET_DIR)/tests/node/integration/ && ./test
	
build-integration-tests: target-dir build-shoggoth-static camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./integration/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/node/integration/test

#========================================================================================

# FUZZ ========================================================================================

run-fuzz-tests:
	cd $(TARGET_DIR)/tests/node/fuzz/ && ./test
	
build-fuzz-tests: target-dir build-shoggoth-static camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./fuzz/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/node/fuzz/test

#========================================================================================

