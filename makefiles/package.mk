ARCH := $(shell uname -m)
OS := $(shell uname -s)

RELEASE_FILENAME = shoggoth-$(PREFIX)-$(VERSION)-$(OS)-$(ARCH).zip

.PHONY: check-path-arg

build-docs: build-static $(STATIC_LIBS)
	echo "Building docs..."

	cp -r ./src/docs/md/ $(TARGET_DIR)/explorer/docs/md/

	$(CC) $(CFLAGS) ./src/docs/main.c ./src/docs/docs_api.c ./src/md_to_html/md_to_html.c ./lib/md4c/src/entity.c ./lib/md4c/src/md4c.c ./lib/md4c/src/md4c-html.c ./target/libshoggoth.a $(STATIC_LIBS) $(LDFLAGS) -o $(TARGET_DIR)/docsgen
	cd $(TARGET_DIR) && ./docsgen

build-explorer: build-static $(STATIC_LIBS)
	echo "Building explorer..."

	rm -rf $(TARGET_DIR)/explorer/

	mkdir -p $(TARGET_DIR)/explorer/
	mkdir -p $(TARGET_DIR)/explorer/docs/

	mkdir -p $(TARGET_DIR)/explorer/out/
	mkdir -p $(TARGET_DIR)/explorer/out/docs/

	cp -r ./src/explorer/templates/ $(TARGET_DIR)/explorer/templates/
	cp -r ./src/explorer/html/ $(TARGET_DIR)/explorer/html/

	$(CC) $(CFLAGS) ./src/explorer/main.c ./target/libshoggoth.a $(STATIC_LIBS) $(LDFLAGS) -o $(TARGET_DIR)/explorergen
	cd $(TARGET_DIR) && ./explorergen

	$(MAKE) build-docs


check-path-arg:
	@if [ -z "$(RP)" ]; then \
		echo "RP is not set. Please specify a runtime path 'RP=<absolute_path> make generate-runtime'"; \
		exit 1; \
	fi

generate-runtime: check-path-arg build-explorer
	mkdir -p $(RP)
	
	mkdir -p $(RP)/bin/
	mkdir -p $(RP)/client/
	mkdir -p $(RP)/scripts/
	mkdir -p $(RP)/node
	
	mkdir -p $(RP)/node/bin $(RP)/node/keys $(RP)/node/tmp $(RP)/node/update
	mkdir -p $(RP)/client/keys $(RP)/client/tmp
	
	mkdir -p $(RP)/node/explorer $(RP)/node/explorer/docs/ $(RP)/node/explorer/static/ $(RP)/node/explorer/templates/
	
	cp ./src/node/default-node-config.toml $(RP)/node/config.toml
	cp ./src/client/default-client-config.toml $(RP)/client/config.toml
	cp ./src/client/default-known-nodes.json $(RP)/client/known_nodes.json

	cp $(TARGET_DIR)/redis-server $(RP)/node/bin/ && cp $(TARGET_DIR)/redisjson.so $(RP)/node/bin/
	cp ./src/node/redis.conf $(RP)/node/

	cp -r $(TARGET_DIR)/explorer/out/* $(RP)/node/explorer/
	cp -r ./src/explorer/static/* $(RP)/node/explorer/static/
	cp -r ./src/explorer/templates/* $(RP)/node/explorer/templates/

package-release:
	$(eval PREFIX = release)

	$(MAKE) build
	
	PREFIX=release $(MAKE) create-package

	cp $(TARGET_DIR)/shog-flat $(TARGET_DIR)/$(PREFIX)-$(VERSION)/bin/shog

	cd $(TARGET_DIR)/$(PREFIX)-$(VERSION)/ && find ./ -exec touch -t 200001011200.00 {} \; && zip -q -rX ../$(RELEASE_FILENAME) ./ 
	
	echo "Finished packaging $(PREFIX) $(VERSION) for $(OS) $(ARCH)"
	echo "Package SHA256 checksum: "
	sha256sum $(TARGET_DIR)/$(RELEASE_FILENAME)


package-dev:
	$(eval PREFIX = dev)

	$(MAKE) build-dev
	
	PREFIX=dev $(MAKE) create-package

	cp $(TARGET_DIR)/shog-sanitized $(TARGET_DIR)/$(PREFIX)-$(VERSION)/bin/shog

	cd $(TARGET_DIR)/$(PREFIX)-$(VERSION)/ && find ./ -exec touch -t 200001011200.00 {} \; && zip -q -rX ../$(RELEASE_FILENAME) ./ 
	
	echo "Finished packaging $(PREFIX) $(VERSION) for $(OS) $(ARCH)"
	echo "Package SHA256 checksum: "
	sha256sum $(TARGET_DIR)/$(RELEASE_FILENAME)


create-package: target-dir
	echo "Packaging $(VERSION) for $(OS) $(ARCH)"

	cd $(TARGET_DIR)/ && rm -rf $(PREFIX)-$(VERSION)/
	
	$(MAKE) generate-runtime RP=$(TARGET_DIR)/$(PREFIX)-$(VERSION)/

	cp ./src/scripts/* $(TARGET_DIR)/$(PREFIX)-$(VERSION)/scripts/
	
	chmod +x $(TARGET_DIR)/$(PREFIX)-$(VERSION)/scripts/install.sh
	chmod +x $(TARGET_DIR)/$(PREFIX)-$(VERSION)/scripts/uninstall.sh
	chmod +x $(TARGET_DIR)/$(PREFIX)-$(VERSION)/scripts/shog.sh


package-source:
	rm -f ./target/shoggoth-source-$(VERSION).zip
	git archive --format=zip --output=./target/shoggoth-source-$(VERSION).zip master