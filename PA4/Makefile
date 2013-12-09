all: server_ client_

CFLAGS=-Wall -Wextra -g3 -ggdb
server_:
	gcc $(CFLAGS) -oserver server.c pa4str.c trie.c trie_set.c -lpthread

client_:
	gcc $(CFLAGS) -o client client_state_machine.c client.c pa4str.c trie.c trie_set.c -lpthread
