MAKEFLAGS += --silent

CFLAGS = -g -std=c11

# warning flags
WARN_CFLAGS = -Wall -Wextra -Wformat -Wformat-security -Warray-bounds -Wconversion
# WARN_CFLAGS += -Werror

ifeq ($(shell uname), Linux)
CFLAGS += -fsanitize=address,undefined,leak,undefined
LDFLAGS += -lasan -lubsan 
endif

CFLAGS += -I ../../lib/netlibc/include
LDFLAGS += -lssl -lcrypto $$(pkg-config --cflags --libs uuid)

TARGET_DIR = ../../target

ROOT_DIR = ../../

# static libraries
STATIC_LIBS = $(TARGET_DIR)/libnetlibc.a $(TARGET_DIR)/sonic-sanitized.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a

CC = gcc
LD = gcc

camel:
	cd $(ROOT_DIR) && $(MAKE) camel

build:
	cd $(ROOT_DIR) && $(MAKE) build-static

target-dir:
	mkdir -p $(TARGET_DIR)
	
	mkdir -p $(TARGET_DIR)/tests
	mkdir -p $(TARGET_DIR)/tests/client/
	
	mkdir -p $(TARGET_DIR)/tests/client/functional

	mkdir -p $(TARGET_DIR)/tests/client/unit
	mkdir -p $(TARGET_DIR)/tests/client/integration
	mkdir -p $(TARGET_DIR)/tests/client/fuzz
	
# FUNCTIONAL ========================================================================================

run-functional-tests:
	cd $(TARGET_DIR)/tests/client/functional/ && ./test
	
build-functional-tests: target-dir build camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./functional/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/test


#========================================================================================


# UNIT ========================================================================================

run-unit-tests:
	cd $(TARGET_DIR)/tests/client/unit/ && ./test
	
build-unit-tests: target-dir build camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./unit/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/client/unit/test

#========================================================================================

# INTEGRATION ========================================================================================

run-integration-tests:
	cd $(TARGET_DIR)/tests/client/integration/ && ./test
	
build-integration-tests: target-dir build camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./integration/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/client/integration/test

#========================================================================================

# FUZZ ========================================================================================

run-fuzz-tests:
	cd $(TARGET_DIR)/tests/client/fuzz/ && ./test
	
build-fuzz-tests: target-dir build camel
	$(CC) $(CFLAGS) $(WARN_CFLAGS) ./fuzz/test.c $(TARGET_DIR)/libshoggoth.a $(STATIC_LIBS) $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/client/fuzz/test

#========================================================================================

