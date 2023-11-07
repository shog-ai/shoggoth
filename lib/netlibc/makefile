VERSION = 0.1.4

MAKEFLAGS += --silent

ifndef CC
CC = gcc
endif

CFLAGS = -g

all: ./target/libnetlibc.a

HEADER_CHECKSUM := $$(md5sum ./include/netlibc.h | cut -d ' ' -f 1)
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
	sudo mkdir -p /usr/local/include/netlibc/
	
	sudo cp -r ./include/netlibc/* /usr/local/include/netlibc/
	sudo cp ./include/netlibc.h /usr/local/include/netlibc.h

	sudo cp ./target/libnetlibc.a /usr/local/lib

uninstall:
	sudo rm -r /usr/local/include/netlibc/
	sudo rm /usr/local/include/netlibc.h
	
	sudo rm /usr/local/lib/libnetlibc.a

./target/libnetlibc.a: target-dir
	$(CC) $(CFLAGS) -c ./src/netlibc.c -o ./target/netlibc.o
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

