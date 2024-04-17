clean:
	rm -rf ./target/*
	$(MAKE) target-dir

version:
	echo $(VERSION)

downstream:
	git fetch && git pull
	

sync: sync-libs-headers

sync-libs-headers:
	cp ./camel/camel.h ./src/include/camel.h
	cp ./sonic/sonic.h ./src/include/sonic.h
	cp ./tuwi/tuwi.h ./src/include/tuwi.h
	cp ./shogdb/src/lib/lib.h ./src/include/shogdb.h

kill-db:
	killall -v -w -s 9 shogdb
