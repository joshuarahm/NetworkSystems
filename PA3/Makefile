CC=gcc
CFLAGS:=-Wall -Wextra -Wno-unused-parameter -g3 -ggdb
OBJS:=
LDFLAGS:=
ROUTER:=router_LS
DEBUG?=0
DEBUGFLAGS=-DDEBUG$(DEBUG)

all: router_LS

router_LS: $(OBJS) router_LS.c
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ $<

#obj.o: obj.c
#	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ -c $<

clean:
	rm -rf router_LS.dSYM/ *.o $(ROUTER)
