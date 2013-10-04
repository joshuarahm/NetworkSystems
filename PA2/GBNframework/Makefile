
CC=gcc
CFLAGS:=-Wall -Wextra -g3 -ggdb
OBJS:=blockqueue.o gbn_read.o gbn_socket.o gbn_write.o
LDFLAGS:=-lpthread
CLIENT:=client

all: $(OBJS) gbn_client.o
	$(CC) $(CFLAGS) -o $(CLIENT) $(OBJS) gbn_client.o $(LDFLAGS)

blockqueue.o: blockqueue.c
	$(CC) $(CFLAGS) -o $@ -c $<

gbn_client.o: gbn_client.c
	$(CC) $(CFLAGS) -o $@ -c $<

gbn_read.o: gbn_read.c
	$(CC) $(CFLAGS) -o $@ -c $<

gbn_socket.o: gbn_socket.c
	$(CC) $(CFLAGS) -o $@ -c $<

gbn_write.o: gbn_write.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm *.o client
