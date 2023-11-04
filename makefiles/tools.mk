clean:
	$(MAKE) clean-shoggoth
	$(MAKE) clean-sonic
	$(MAKE) clean-camel
	$(MAKE) clean-tuwi
# $(MAKE) clean-redis

clean-shoggoth: target-dir
	rm -rf ./target/*
	$(MAKE) target-dir

clean-sonic:
	cd lib/sonic && $(MAKE) clean

clean-camel:
	cd lib/sonic && $(MAKE) clean

clean-tuwi:
	cd lib/sonic && $(MAKE) clean

clean-redis:
	cd lib/redis && $(MAKE) clean

version:
	echo $(VERSION)

downstream:
	git fetch && git pull
	
upstream:
	git add .
	@read -p "Enter commit message: " message; \
	git commit -m "$$message"
	git push


sync: sync-camel sync-sonic sync-tuwi sync-libs-headers

sync-camel:
	rm -rf ./lib/camel/
	rsync -av --exclude='target' --exclude='.git' ../camel/ ./lib/camel/

sync-sonic:
	rm -rf ./lib/sonic/
	rsync -av --exclude='target' --exclude='.git' ../sonic/ ./lib/sonic/

sync-tuwi:
	rm -rf ./lib/tuwi/
	rsync -av --exclude='target' --exclude='.git' ../tuwi/ ./lib/tuwi/

sync-libs-headers:
	cp ./lib/camel/camel.h ./src/include/camel.h
	cp ./lib/sonic/sonic.h ./src/include/sonic.h
	cp ./lib/tuwi/tuwi.h ./src/include/tuwi.h

kill-redis:
	killall -v -w -s 9 redis-server
