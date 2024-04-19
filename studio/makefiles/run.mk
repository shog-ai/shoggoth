START_FLAGS = 

# NODE =======================================================================================================
run: dev
	echo "\n\n"	
	cd $(TARGET_DIR)/dev-$(VERSION)/ && ./bin/studio run -r $$(pwd)

run-flat: all
	echo "\n\n"	
	cd $(TARGET_DIR)/release-$(VERSION)/ && ./bin/studio run -r $$(pwd)

# ============================================================================================================

