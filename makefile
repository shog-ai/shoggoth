MAKEFLAGS += --silent

VERSION := v$(shell cat version.txt)

# compiler and linker
ifndef CC
CC = gcc
endif
LD = $(CC)

# flags
CFLAGS = -g -std=c11 -D_GNU_SOURCE -Wno-unused-value -Wno-format-zero-length $$(pkg-config --cflags openssl)  -I ./netlibc/include
CFLAGS_FLAT = -DNDEBUG
ifdef ANDROID
CFLAGS += -D__android__
endif
ifeq ($(CC), gcc)
CFLAGS += -Wno-format-overflow -Wno-format-truncation
else ifeq (gcc, $(shell if [ "$$(cc --help 2>&1 | grep -o -m 1 'gcc')" = "gcc" ]; then echo "gcc" ; elif [ "$$(cc --help 2>&1 | grep -o -m 1 'clang')" = "clang" ]; then echo "clang"; else echo "NONE"; fi))
CFLAGS += -Wno-format-overflow -Wno-format-truncation
endif

LDFLAGS = $$(pkg-config --libs openssl) $$(pkg-config --cflags --libs uuid) -pthread -lm -ljansson

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

all: build-flat
dev: build-debug


.PHONY: clean version app build-flat

check-cc:
	echo $(shell if [ "$$(cc --help 2>&1 | grep -o -m 1 'gcc')" = "gcc" ]; then echo "gcc" ; elif [ "$$(cc --help 2>&1 | grep -o -m 1 'clang')" = "clang" ]; then echo "clang"; else echo "NONE"; fi)

build-debug:
	cd cli && cargo build
	cd cli && cp ./target/debug/shog ./target/shog

build-flat:
	cd cli && cargo build --release
	cd cli && cp ./target/release/shog ./target/shog

clear-runtime:
	rm -rf ./cli/target/runtime

clear-home-runtime:
	rm -rf ~/shog/

run: dev
	# make clear-runtime
	mkdir -p ./cli/target/runtime
	cd ./cli/target && ./shog run -r ./runtime

clean:
	cd app && cargo clean
	cd cli && cargo clean
	cd studio && cargo clean


install:
ifneq ($(wildcard ~/shog/),)
	echo "shog is already installed in $$HOME/shog/"
	echo "moving $$HOME/shog/ to $$HOME/shog_old/"
	rm -rf ~/shog_old/
	mv ~/shog/ ~/shog_old/
endif
	rm -rf ~/shog/ && mkdir ~/shog/
		
	sudo cp ./cli/target/shog /usr/local/bin/shog


app:
	cd ./app && cargo tauri build
	cp -r ./app/target/release/bundle/ ./app/target/bundles/
	
run-app:
	# make clear-home-runtime
	cd ./app && cargo tauri dev
	cd ./app/target/release && ./shog-studio
