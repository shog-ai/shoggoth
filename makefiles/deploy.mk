.PHONY: deploy

deploy-nodes: package-dev
	rm -rf ./target/shoggoth-deploy
	mkdir -p ./target/shoggoth-deploy

	cp -r $(TARGET_DIR)/dev-$(VERSION)/ ./target/shoggoth-deploy/1
	cp -r $(TARGET_DIR)/dev-$(VERSION)/ ./target/shoggoth-deploy/2
	cp -r $(TARGET_DIR)/dev-$(VERSION)/ ./target/shoggoth-deploy/3

	cp ./deploy/1/public.txt ./target/shoggoth-deploy/1/node/keys/ && cp ./deploy/1/private.txt ./target/shoggoth-deploy/1/node/keys/
	cp ./deploy/2/public.txt ./target/shoggoth-deploy/2/node/keys/ && cp ./deploy/2/private.txt ./target/shoggoth-deploy/2/node/keys/
	cp ./deploy/3/public.txt ./target/shoggoth-deploy/3/node/keys/ && cp ./deploy/3/private.txt ./target/shoggoth-deploy/3/node/keys/

	cp ./deploy/1/1.toml ./target/shoggoth-deploy/1/node/config.toml
	cp ./deploy/2/2.toml ./target/shoggoth-deploy/2/node/config.toml
	cp ./deploy/3/3.toml ./target/shoggoth-deploy/3/node/config.toml

	cp ./deploy/1/dbconfig.toml ./target/shoggoth-deploy/1/node/dbconfig.toml
	cp ./deploy/2/dbconfig.toml ./target/shoggoth-deploy/2/node/dbconfig.toml
	cp ./deploy/3/dbconfig.toml ./target/shoggoth-deploy/3/node/dbconfig.toml
	
deploy:
	cd $(TARGET_DIR) && mprocs "sleep 1 && ./shog-sanitized run -r ./shoggoth-deploy/1" "./shog-sanitized run -r ./shoggoth-deploy/2" "sleep 2 && ./shog-sanitized run -r ./shoggoth-deploy/3"

# START_FLAGS = LD_PRELOAD=$$(gcc -print-file-name=libasan.so)
START_FLAGS = 

deploy-remote: package-release
	echo "Copying package to $(A):/home/$(U)/"

	mkdir -p ./target/remote-deploy/
	cp ./target/$(RELEASE_FILENAME) ./target/remote-deploy/shoggoth.zip
	cp $(C) ./target/remote-deploy/config.toml
	
	scp -r ./target/remote-deploy/ $(U)@$(A):~/deployment
	ssh -t $(U)@$(A) 'unzip -o -q ~/deployment/shoggoth.zip -d ~/shoggoth/ && cd ~/shoggoth/ && ./scripts/install.sh && cd ~ && cp ~/deployment/config.toml ~/shoggoth/node/config.toml && rm -r ~/deployment'

deploy1:
	cd $(TARGET_DIR) && $(START_FLAGS) ./shog-sanitized run -r ./shoggoth-deploy/1

deploy2:
	cd $(TARGET_DIR) && $(START_FLAGS) ./shog-sanitized run -r ./shoggoth-deploy/2
	
deploy3:
	cd $(TARGET_DIR) && $(START_FLAGS) ./shog-sanitized run -r ./shoggoth-deploy/3
