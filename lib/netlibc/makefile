VERSION = 0.1.4

MAKEFLAGS += --silent

ifndef CC
CC = gcc
endif

all: ./target/libnetlibc.a

# check-netlibc:
# ifeq (,$(wildcard /usr/local/include/netlibc.h ))
# 	echo "Installing netlibc ..."
# 	$(MAKE)
# 	$(MAKE) install
# endif

# LIBRARY_CHECKSUM := 872ba3160577be3d08661bedfbb69e74

HEADER_CHECKSUM := $$(md5sum ./netlibc.h | cut -d ' ' -f 1)
LIBRARY_CHECKSUM := $$(md5sum ./target/libnetlibc.a | cut -d ' ' -f 1)

check-netlibc:
	$(MAKE)

	if [ -f /usr/local/include/netlibc.h ]; then \
	    INSTALLED_HEADER_CHECKSUM=$$(md5sum /usr/local/include/netlibc.h | cut -d ' ' -f 1); \
	    if [ "$$INSTALLED_HEADER_CHECKSUM" != "$(HEADER_CHECKSUM)" ]; then \
	        echo "netlibc header checksum mismatch. Reinstalling..."; \
	        $(MAKE) install; \
	    fi; \
	else \
	    echo "netlibc header not found. Installing..."; \
	    $(MAKE) install; \
	fi


	if [ -f /usr/local/lib/libnetlibc.a ]; then \
	    INSTALLED_LIBRARY_CHECKSUM=$$(md5sum /usr/local/lib/libnetlibc.a | cut -d ' ' -f 1); \
	    if [ "$$INSTALLED_LIBRARY_CHECKSUM" != "$(LIBRARY_CHECKSUM)" ]; then \
	        echo "netlibc library checksum mismatch. Reinstalling..."; \
	        $(MAKE) install; \
	    fi; \
	else \
	    echo "netlibc library not found. Installing..."; \
	    $(MAKE) install; \
	fi


install:
	sudo cp ./netlibc.h /usr/local/include
	sudo cp ./target/libnetlibc.a /usr/local/lib

uninstall:
	sudo rm /usr/local/include/netlibc.h
	sudo rm /usr/local/lib/libnetlibc.a

./target/libnetlibc.a: target-dir
	$(CC) -c ./src/netlibc.c -o ./target/netlibc.o
	ar rcs ./target/libnetlibc.a ./target/netlibc.o

target-dir:
	mkdir -p target
	mkdir -p target/examples


run-example: target-dir ./target/libnetlibc.a
	$(CC) $(CFLAGS) ./examples/$(E).c ./target/libnetlibc.a -o ./target/examples/$(E)
	cd target/examples/ && ./$(E)

clean:
	rm -rf ./target/

downstream:
	git fetch && git pull
	
upstream:
	git add .
	@read -p "Enter commit message: " message; \
	git commit -m "$$message"
	git push

