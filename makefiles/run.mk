START_FLAGS = 

prepare-runtime:
	cd $(TARGET_DIR) && mkdir -p dev-runtime

# NODE =======================================================================================================
run-node: dev
	cp ./deploy/1/private.txt $(TARGET_DIR)/dev-$(VERSION)/node/keys/private.txt && cp ./deploy/1/public.txt $(TARGET_DIR)/dev-$(VERSION)/node/keys/public.txt
	
	echo "\n\n"	
	cd $(TARGET_DIR)/dev-$(VERSION)/ && ./bin/shog run -r $$(pwd) -c ../../deploy/2/2.toml

# ============================================================================================================

# STUDIO =======================================================================================================
run-studio: dev
	echo "\n\n"	
	cd $(TARGET_DIR)/dev-$(VERSION)/ && ./bin/shog studio -r $$(pwd) -c ../../deploy/2/2.toml

# ============================================================================================================

