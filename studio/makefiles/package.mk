ARCH := $(shell uname -m)
OS := $(shell uname -s)

RELEASE_FILENAME = studio-$(PREFIX)-$(VERSION)-$(OS)-$(ARCH).zip

.PHONY: check-path-arg

check-path-arg:
	@if [ -z "$(RP)" ]; then \
		echo "RP is not set. Please specify a runtime path 'RP=<absolute_path> make generate-runtime'"; \
		exit 1; \
	fi

generate-runtime: check-path-arg 
	mkdir -p $(RP)
	
	mkdir -p $(RP)/bin/
	mkdir -p $(RP)/scripts/
	mkdir -p $(RP)/studio
	
	mkdir -p $(RP)/studio/static $(RP)/studio/models $(RP)/studio/templates/ $(RP)/studio/html/ $(RP)/studio/model_server

# cp $(TARGET_DIR)/model_server $(RP)/bin/shog_model_server
# cp ./lib/llamacpp/ggml-metal.metal $(RP)/studio/model_server/

	cp -r ./src/static/* $(RP)/studio/static/
	cp -r ./src/html/* $(RP)/studio/html/
	cp -r ./src/templates/* $(RP)/studio/templates/
	
package-release:
	$(eval PREFIX = release)

	$(MAKE) build-flat
	
	PREFIX=release $(MAKE) create-package

	cp $(TARGET_DIR)/studio-flat $(TARGET_DIR)/$(PREFIX)-$(VERSION)/bin/studio

	cd $(TARGET_DIR)/$(PREFIX)-$(VERSION)/ && find ./ -exec touch -t 200001011200.00 {} \; && zip -q -rX ../$(RELEASE_FILENAME) ./ 
	
	echo "Finished packaging $(PREFIX) $(VERSION) for $(OS) $(ARCH)"
	echo "Package SHA256 checksum: "
	sha256sum $(TARGET_DIR)/$(RELEASE_FILENAME)


package-dev:
	$(eval PREFIX = dev)

	$(MAKE) build-sanitized
	
	PREFIX=dev $(MAKE) create-package

	cp $(TARGET_DIR)/studio-sanitized $(TARGET_DIR)/$(PREFIX)-$(VERSION)/bin/studio

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
	chmod +x $(TARGET_DIR)/$(PREFIX)-$(VERSION)/scripts/shogstudio.sh


