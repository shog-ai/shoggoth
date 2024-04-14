test-libs:
	cd ./sonic && make test

test: build-tests run-tests

build-tests: build-unit-tests build-integration-tests build-functional-tests build-fuzz-tests
run-tests: run-unit-tests run-integration-tests run-fuzz-tests run-functional-tests

build-unit-tests:
	cd tests/node/ && $(MAKE) build-unit-tests

run-unit-tests:
	cd tests/node/ && $(MAKE) run-unit-tests

build-integration-tests:
	cd tests/node/ && $(MAKE) build-integration-tests

run-integration-tests:
	cd tests/node/ && $(MAKE) run-integration-tests

build-functional-tests:
	cd tests/node/ && $(MAKE) build-functional-tests

run-functional-tests:
	cd tests/node/ && $(MAKE) run-functional-tests

build-fuzz-tests:
	cd tests/node/ && $(MAKE) build-fuzz-tests

run-fuzz-tests:
	cd tests/node/ && $(MAKE) run-fuzz-tests

	
