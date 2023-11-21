MAKEFLAGS += --silent

VERSION = v0.1.6

TARGET = shog
SRC_DIR = ./src
NODE_SRC_DIR = ./src/node
CLIENT_SRC_DIR = ./src/client

TARGET_DIR = ./target

OBJ_DIR = $(TARGET_DIR)

OBJS += $(TARGET_DIR)/args.o $(TARGET_DIR)/json.o $(TARGET_DIR)/toml.o $(TARGET_DIR)/profile.o $(TARGET_DIR)/utils.o $(TARGET_DIR)/openssl.o $(TARGET_DIR)/client.o $(TARGET_DIR)/clone.o
STATIC_LIBS = $(TARGET_DIR)/libnetlibc.a $(TARGET_DIR)/sonic.a $(TARGET_DIR)/tuwi.a $(TARGET_DIR)/cjson.a $(TARGET_DIR)/tomlc.a

CFLAGS += -I ./target/openssl/include/
LDFLAGS += -lws2_32

CC = x86_64-w64-mingw32-gcc

windows:
	echo "Cross compiling Shoggoth for Windows"

	rm -rf ./target/openssl

	unzip -o -q ./lib/openssl-1.1.1.zip -d ./target/
	cd ./target/openssl/ && perl ./Configure mingw64 --cross-compile-prefix=x86_64-w64-mingw32-
	
	- cd ./target/openssl/ && make

	make build -f makefiles/windows.mk


build:
	cd ./target && rm -f ./*.o
	
	make -f makefiles/windows.mk build-objects-flat
	make -f makefiles/windows.mk link-objects-flat


build-objects-flat:
	echo "Building Shoggoth (WINDOWS) (flat)..."

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -DVERSION="\"$(VERSION)\"" -c $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/args/args.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/json/json.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/toml/toml.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/profile/profile.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(SRC_DIR)/utils/utils.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -Wno-deprecated-declarations -c $(SRC_DIR)/openssl/openssl.c 

	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/client.c
	$(CC) $(CFLAGS) $(CFLAGS_FLAT) $(WARN_CFLAGS) -c $(CLIENT_SRC_DIR)/clone.c

	mv ./*.o $(TARGET_DIR)

link-objects-flat: $(STATIC_LIBS)
	echo "Linking Shoggoth (WINDOWS) (flat)..."
	
	$(LD) $(OBJS) $(MAIN_OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $(TARGET_DIR)/$(TARGET)-flat 
	strip $(TARGET_DIR)/$(TARGET)-flat


