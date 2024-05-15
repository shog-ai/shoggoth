DBG_FLG = -Wall -Wextra -pedantic

ifdef debug
DBG_FLG += -g3 -Og -D_DEBUG -fsanitize=address -fno-omit-frame-pointer
LDFLAGS += -lasan
endif

CFLAGS = -std=c11 $(DBG_FLG)


all: janssonpath.o

janssonpath.o: janssonpath_impl.o parse.o
	ld -r $? -o $@

clean:
	rm *.o