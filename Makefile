CFLAGS = -Wall -Werror -Wextra -Wpedantic -std=c99 -g

ifdef debug
	CFLAGS += -DHLIB_DEBUG -fsanitize=undefined -fsanitize=address -fsanitize=leak
endif

all: hfinance

hfinance: hlib.o hfinance.c
	cc $(CFLAGS) -o hfinance hfinance.c hlib.o

hlib.o: $(wildcard hlib/*.c)
	cc $(CFLAGS) -c hlib/hlib.c -o hlib.o

clean:
	rm -f *.o *_test
