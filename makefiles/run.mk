START_FLAGS = 

prepare-runtime:
	cd $(TARGET_DIR) && mkdir -p dev-runtime

# NODE =======================================================================================================
run-node: dev
	cp ./deploy/1/private.txt $(TARGET_DIR)/dev-$(VERSION)/node/keys/private.txt && cp ./deploy/1/public.txt $(TARGET_DIR)/dev-$(VERSION)/node/keys/public.txt
	
	echo "\n\n"	
	cd $(TARGET_DIR)/dev-$(VERSION)/ && ./bin/shog node run -r $$(pwd) -c ../../deploy/2/2.toml


run-node-valgrind: build-debug
	cd $(TARGET_DIR) && valgrind --leak-check=full ./shog-dev

run-node-gdb: build-debug
	cd $(TARGET_DIR) && gdb ./shog-dev
# ============================================================================================================

# CLIENT =======================================================================================================
run-client:
	cp ./deploy/1/private.txt $(TARGET_DIR)/dev-$(VERSION)/client/keys/private.txt && cp ./deploy/1/public.txt $(TARGET_DIR)/dev-$(VERSION)/client/keys/public.txt
	cp ./deploy/known_nodes.json $(TARGET_DIR)/dev-$(VERSION)/client/known_nodes.json
	
	echo "\n\n"	
	cd $(TARGET_DIR)/dev-$(VERSION)/ && ./bin/shog client init -r $$(pwd)
		
	echo "\n\n"	
	cd $(TARGET_DIR)/dev-$(VERSION)/shoggoth-profile && ../bin/shog client publish -r $$(pwd)/../


# ============================================================================================================

