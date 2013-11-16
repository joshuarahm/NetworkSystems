CC=gcc
CFLAGS:=-Wall -Wextra -Wno-unused-parameter -g3 -ggdb
OBJS:=router.o logging.o routed_LS.o
LDFLAGS:=
ROUTER:=routed_LS
TEST:=routertest
TOBJS:=router.o logging.o routertest.o
DEBUG?=1
DEBUGFLAGS=-DDEBUG$(DEBUG)

all: routed_LS
test: routertest

$(ROUTER): $(OBJS)
	$(CC) -o $@ $(OBJS) -lpthread

$(TEST): $(TOBJS)
	$(CC) -o $@ $(TOBJS) -lpthread

routertest.o: routertest.c
	$(CC) $(CFLAGS) -o$@ -c $< $(DEBUGFLAGS)

routed_LS.o: routed_LS.c
	$(CC) $(CFLAGS) -o$@ -c $< $(DEBUGFLAGS)

router.o: router.c
	$(CC) $(CFLAGS) -o$@ -c $< $(DEBUGFLAGS)

logging.o: logging.c
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ -c $<

#obj.o: obj.c
#	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ -c $<

clean:
	- rm -rf *.o $(ROUTER) $(TEST)
	- rm -rf routed_LS.dSYM/
