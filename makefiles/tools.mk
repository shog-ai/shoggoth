clean:
	$(MAKE) clean-shoggoth
	$(MAKE) clean-shogdb
	$(MAKE) clean-sonic
	$(MAKE) clean-camel
	$(MAKE) clean-tuwi
	$(MAKE) clean-netlibc
	$(MAKE) clean-tomlc
	$(MAKE) clean-cjson

clean-shoggoth: target-dir
	rm -rf ./target/*
	$(MAKE) target-dir

clean-sonic:
	cd lib/sonic && $(MAKE) clean

clean-camel:
	cd lib/sonic && $(MAKE) clean

clean-tuwi:
	cd lib/sonic && $(MAKE) clean

clean-netlibc:
	cd lib/netlibc && $(MAKE) clean

clean-tomlc:
	cd lib/tomlc && $(MAKE) clean

clean-cjson:
	cd lib/cjson && $(MAKE) clean

clean-shogdb:
	cd lib/shogdb && $(MAKE) clean

version:
	echo $(VERSION)

downstream:
	git fetch && git pull
	

sync: sync-libs-headers

sync-libs-headers:
	cp ./lib/camel/camel.h ./src/include/camel.h
	cp ./lib/sonic/sonic.h ./src/include/sonic.h
	cp ./lib/tuwi/tuwi.h ./src/include/tuwi.h
	cp ./lib/shogdb/src/lib/lib.h ./src/include/shogdb.h

kill-db:
	killall -v -w -s 9 shogdb
