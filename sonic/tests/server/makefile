ifeq ($(shell uname), Linux)
CFLAGS += -fsanitize=address,undefined,leak,undefined
LDFLAGS += -lasan -lubsan 
endif

CFLAGS += -g -I ../../../netlibc/include
LDFLAGS += $$(pkg-config --libs openssl) $(TARGET_DIR)/libnetlibc.a


TARGET_DIR = ../../target

ROOT_DIR = ../../

CC = gcc
LD = gcc

LIBSONIC = $(TARGET_DIR)/libsonic-sanitized.a

camel:
	cd $(ROOT_DIR) && $(MAKE) camel

build: $(TARGET_DIR)/libnetlibc.a
	cd $(ROOT_DIR) && $(MAKE) build-sanitized

target-dir:
	mkdir -p $(TARGET_DIR)
	
	mkdir -p $(TARGET_DIR)/tests
	mkdir -p $(TARGET_DIR)/tests/server/
	
	mkdir -p $(TARGET_DIR)/tests/server/functional
	mkdir -p $(TARGET_DIR)/tests/server/functional/serve

	mkdir -p $(TARGET_DIR)/tests/server/unit
	mkdir -p $(TARGET_DIR)/tests/server/integration
	mkdir -p $(TARGET_DIR)/tests/server/fuzz


$(TARGET_DIR)/libnetlibc.a:
	cd ../../../netlibc/ && make
	cp ../../../netlibc/target/libnetlibc.a $(TARGET_DIR)/libnetlibc.a


# FUNCTIONAL ========================================================================================

$(TARGET_DIR)/tests/server/functional/serve/large_test_file.bin:
	head -c 500000000 </dev/urandom > $@
	
large-test-file: $(TARGET_DIR)/tests/server/functional/serve/large_test_file.bin

run-functional-tests:
	cd $(TARGET_DIR)/tests/server/functional/serve/ && ./test
	
build-functional-tests: target-dir build camel large-test-file
	$(MAKE) build-functional-test-serve

build-functional-test-serve:
	$(CC) $(CFLAGS) ./functional/serve/test.c $(TARGET_DIR)/camel.a $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/server/functional/serve/test 
	$(CC) $(CFLAGS) ./functional/serve/server.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/server/functional/serve/server

#========================================================================================


# UNIT ========================================================================================

run-unit-tests:
	cd $(TARGET_DIR)/tests/server/unit/ && ./test_server
	
build-unit-tests: target-dir build camel
	$(CC) $(CFLAGS) ./unit/test_server.c $(TARGET_DIR)/camel.a $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/server/unit/test_server 
	
#========================================================================================

# INTEGRATION ========================================================================================

run-integration-tests:
	cd $(TARGET_DIR)/tests/server/integration/ && ./test
	
build-integration-tests: target-dir build camel
	$(CC) $(CFLAGS) ./integration/test.c $(TARGET_DIR)/camel.a $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/server/integration/test 
	
#========================================================================================

# FUZZ ========================================================================================

run-fuzz-tests:
	cd $(TARGET_DIR)/tests/server/fuzz/ && ./test
	
build-fuzz-tests: target-dir build camel
	$(CC) $(CFLAGS) ./fuzz/test.c $(TARGET_DIR)/camel.a $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/server/fuzz/test 
	
#========================================================================================

