#include "trie_set.h"

#include <stdlib.h>

int trie_set_contains( const trie_set_t* set, const char* key ) {
    trie_t* casted = (trie_t*)set;
    return trie_get( casted, key ) != NULL ;
}

void trie_set_insert( trie_set_t* set, const char* key ) {
    trie_put( set, key, (void*)0x1 );
}

void trie_set_remove( trie_set_t* set, const char* key ) {
    trie_put( set, key, (void*)0x0 );
}
