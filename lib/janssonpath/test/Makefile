DBG_FLG = -Wall -Wextra -pedantic -g3 -Og -D_DEBUG  -fsanitize=address -fno-omit-frame-pointer
CFLAGS = -std=c11 $(DBG_FLG)
LDFLAGS = -lasan -ljansson

test:test.c ../janssonpath.o
	cc $(CFLAGS) $? -o $@ $(LDFLAGS)

../janssonpath.o: ../janssonpath_impl.o ../parse.o
	ld -r $? -o $@