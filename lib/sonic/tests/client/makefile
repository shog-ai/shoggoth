ifeq ($(shell uname), Linux)
CFLAGS += -fsanitize=address,undefined,leak,undefined
LDFLAGS += -lasan -lubsan 
endif

CFLAGS +=
LDFLAGS += -lssl -lcrypto

TARGET_DIR = ../../target

ROOT_DIR = ../../

CC = gcc
LD = gcc

LIBSONIC = $(TARGET_DIR)/libsonic-sanitized.a

camel:
	cd $(ROOT_DIR) && $(MAKE) camel

build:
	cd $(ROOT_DIR) && $(MAKE) build-sanitized

target-dir:
	mkdir -p $(TARGET_DIR)
	
	mkdir -p $(TARGET_DIR)/tests
	mkdir -p $(TARGET_DIR)/tests/client/
	
	mkdir -p $(TARGET_DIR)/tests/client/functional
	mkdir -p $(TARGET_DIR)/tests/client/functional/local_requests
	mkdir -p $(TARGET_DIR)/tests/client/functional/internet_requests
	mkdir -p $(TARGET_DIR)/tests/client/functional/redirects

	mkdir -p $(TARGET_DIR)/tests/client/unit
	mkdir -p $(TARGET_DIR)/tests/client/integration
	mkdir -p $(TARGET_DIR)/tests/client/fuzz
	
# FUNCTIONAL ========================================================================================

$(TARGET_DIR)/tests/client/functional/local_requests/large_test_file.bin:
	head -c 500000000 </dev/urandom > $@
	
large-test-file: $(TARGET_DIR)/tests/client/functional/local_requests/large_test_file.bin

run-functional-tests:
	cd $(TARGET_DIR)/tests/client/functional/local_requests/ && ./test
	cd $(TARGET_DIR)/tests/client/functional/redirects/ && ./test
	cd $(TARGET_DIR)/tests/client/functional/internet_requests/ && ./test -e
	
build-functional-tests: target-dir build camel large-test-file
	$(MAKE) build-functional-test-local-requests
	$(MAKE) build-functional-test-redirects
	$(MAKE) build-functional-test-internet-requests

build-functional-test-local-requests:
	$(CC) -g ./functional/local_requests/test.c $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/local_requests/test 
	$(CC) -g ./functional/local_requests/local_server.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/local_requests/local_server
	
	$(CC) -g ./functional/local_requests/get_200.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/local_requests/get_200
	$(CC) -g ./functional/local_requests/get_404.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/local_requests/get_404
	$(CC) -g ./functional/local_requests/post_with_body.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/local_requests/post_with_body
	$(CC) -g ./functional/local_requests/post_with_headers.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/local_requests/post_with_headers
	$(CC) -g ./functional/local_requests/receive_headers.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/local_requests/receive_headers
	$(CC) -g ./functional/local_requests/response_callback.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/local_requests/response_callback

build-functional-test-internet-requests:
	$(CC) -g ./functional/internet_requests/test.c $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/internet_requests/test 
	
	$(CC) -g ./functional/internet_requests/get_200.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/internet_requests/get_200
	$(CC) -g ./functional/internet_requests/with_subdomain.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/internet_requests/with_subdomain
	$(CC) -g ./functional/internet_requests/ip_and_port.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/internet_requests/ip_and_port
	$(CC) -g ./functional/internet_requests/no_https.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/internet_requests/no_https

build-functional-test-redirects:
	$(CC) -g ./functional/redirects/test.c $(TARGET_DIR)/camel.a $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/redirects/test 
	$(CC) -g ./functional/redirects/local_server.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/redirects/local_server
	
	$(CC) -g ./functional/redirects/many_redirects.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/redirects/many_redirects
	$(CC) -g ./functional/redirects/hops_limit.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/redirects/hops_limit
	$(CC) -g ./functional/redirects/no_redirect.c $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/functional/redirects/no_redirect

#========================================================================================


# UNIT ========================================================================================

run-unit-tests:
	cd $(TARGET_DIR)/tests/client/unit/ && ./test_client
	
build-unit-tests: target-dir build camel
	$(CC) -g ./unit/test_client.c $(TARGET_DIR)/camel.a $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/unit/test_client 
	
#========================================================================================

# INTEGRATION ========================================================================================

run-integration-tests:
	cd $(TARGET_DIR)/tests/client/integration/ && ./test
	
build-integration-tests: target-dir build camel
	$(CC) -g ./integration/test.c $(TARGET_DIR)/camel.a $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/integration/test 
	
#========================================================================================

# FUZZ ========================================================================================

run-fuzz-tests:
	cd $(TARGET_DIR)/tests/client/fuzz/ && ./test
	
build-fuzz-tests: target-dir build camel
	$(CC) -g ./fuzz/test.c $(TARGET_DIR)/camel.a $(LIBSONIC) $(LDFLAGS) -o $(TARGET_DIR)/tests/client/fuzz/test 
	
#========================================================================================

