CC=gcc
CFLAGS:=-Wall -Wextra -Wno-unused-parameter -g3 -ggdb
OBJS:=
LDFLAGS:=
ROUTER:=routed_LS
DEBUG?=0
DEBUGFLAGS=-DDEBUG$(DEBUG)

all: routed_LS

routed_LS: $(OBJS) routed_LS.c
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ $<

#obj.o: obj.c
#	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ -c $<

clean:
	rm -rf routed_LS.dSYM/ *.o $(ROUTER)
