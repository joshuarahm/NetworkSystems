CC=gcc
CFLAGS:=-Wall -Wextra -Wno-unused-parameter -g3 -ggdb
OBJS:=router.o
LDFLAGS:=
ROUTER:=routed_LS
DEBUG?=0
DEBUGFLAGS=-DDEBUG$(DEBUG)

all: routed_LS

$(ROUTER): routed_LS.c $(OBJS)
	$(CC) -o $@ $< $(OBJS)

router.o: router.c
	$(CC) $(CFLAGS) -o$@ -c $< $(DEBUGFLAGS)

#obj.o: obj.c
#	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ -c $<

clean:
	- rm -rf *.o $(ROUTER)
	- rm -rf routed_LS.dSYM/
