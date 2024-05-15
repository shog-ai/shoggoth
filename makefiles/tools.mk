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
	cd ./sonic && $(MAKE) clean

clean-camel:
	cd ./camel && $(MAKE) clean

clean-tuwi:
	cd ./tuwi && $(MAKE) clean

clean-netlibc:
	cd ./netlibc && $(MAKE) clean

clean-tomlc:
	cd lib/tomlc && $(MAKE) clean

clean-cjson:
	cd lib/cjson && $(MAKE) clean

clean-shogdb:
	cd ./shogdb && $(MAKE) clean

version:
	echo $(VERSION)

downstream:
	git fetch && git pull
	

sync: sync-libs-headers

sync-libs-headers:
	cp ./camel/camel.h ./src/include/camel.h
	cp ./sonic/sonic.h ./src/include/sonic.h
	cp ./tuwi/tuwi.h ./src/include/tuwi.h
	cp ./shogdb/src/client/client.h ./src/include/shogdb-client.h

kill-db:
	killall -v -w -s 9 shogdb
